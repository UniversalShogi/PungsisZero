#ifndef MCTSMODEL_H
#define MCTSMODEL_H

#include <torch/torch.h>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

using namespace torch::nn;

class GlobalAvgMaxPool2dImpl : public Module {
    public:
    MaxPool2d maxHead;
    AvgPool2d avgHead;
    Flatten flattener;

    GlobalAvgMaxPool2dImpl() : maxHead(register_module("maxHead", MaxPool2d(MaxPool2dOptions(9)))),
        avgHead(register_module("avgHead", AvgPool2d(AvgPool2dOptions(9)))),
        flattener(register_module("flattener", Flatten(FlattenOptions(FlattenOptions().start_dim(1))))) {}
    
    torch::Tensor forward(const torch::Tensor& input) {
        return flattener(torch::cat({maxHead(input), avgHead(input)}, 1));
    }
};
TORCH_MODULE(GlobalAvgMaxPool2d);

class GlobalBiasStructureImpl : public Module {
    public:
    Sequential biasHead;

    GlobalBiasStructureImpl(int xChannels, int gChannels) : biasHead(register_module("biasHead", Sequential(
        GlobalAvgMaxPool2d(),
        Linear(LinearOptions(2 * gChannels, xChannels))
    ))) {}

    torch::Tensor forward(const torch::Tensor& X, const torch::Tensor& G) {
        return X + biasHead->forward(G).unsqueeze(2).unsqueeze(3);
    }
};
TORCH_MODULE(GlobalBiasStructure);

class AutoSplittingGBSImpl : public Module {
    public:
    int inChannels;
    int gChannels;
    GlobalBiasStructure gbsHead;

    AutoSplittingGBSImpl(int inChannels, int gChannels) : inChannels(inChannels), gChannels(gChannels), gbsHead(register_module("gbsHead"
        , GlobalBiasStructure(inChannels - gChannels, gChannels))) {}
    
    torch::Tensor forward(const torch::Tensor& input) {
        std::vector<torch::Tensor> splitted = torch::split_with_sizes(input, {inChannels - gChannels, gChannels}, 1);
        return gbsHead->forward(splitted[0], splitted[1]);
    }
};
TORCH_MODULE(AutoSplittingGBS);

class AutoTwinGBSImpl : public Module {
    public:
    Conv2d twinConv[2];
    GlobalBiasStructure gbsHead;

    AutoTwinGBSImpl(int inChannels, int gChannels) : twinConv({
        register_module("conv1", Conv2d(Conv2dOptions(inChannels, gChannels, 1).stride(1))),
        register_module("conv2", Conv2d(Conv2dOptions(inChannels, gChannels, 1).stride(1))),
    }), gbsHead(register_module("gbsHead", GlobalBiasStructure(gChannels, gChannels))) {}

    torch::Tensor forward(const torch::Tensor& input) {
        return gbsHead->forward(twinConv[0]->forward(input), twinConv[1]->forward(input));
    }
};
TORCH_MODULE(AutoTwinGBS);

class MCTSModelImpl : public Module {
    public:
    Conv2d binputLayer;
    Linear ninputLayer;
    std::vector<Sequential> resBlocks;
    Sequential trunkEnd;
    Sequential policyHead;
    Sequential valueHead;
    Softmax wdlProb;
    Tanh activateDFPN; // In this position, can DFPN find mate within 100K nodes?

    MCTSModelImpl(int blocks, int channels, int poolBlocks, int poolTrunk, int policyHeadChannels, int valueHeadChannels, int finalValueHeadChannels) :
        binputLayer(register_module("binputLayer", Conv2d(Conv2dOptions(91, channels, 5).stride(1).padding(2)))),
        ninputLayer(register_module("ninputLayer", Linear(LinearOptions(42, channels)))),
        trunkEnd(register_module("trunkEnd", Sequential(
            BatchNorm2d(BatchNorm2dOptions(channels)),
            ReLU()
        ))),
        policyHead(register_module("policyHead", Sequential(
            AutoTwinGBS(channels, policyHeadChannels),
            BatchNorm2d(policyHeadChannels),
            ReLU(),
            Conv2d(Conv2dOptions(policyHeadChannels, 139, 1).stride(1)),
            Flatten(FlattenOptions().start_dim(1)),
            Softmax(SoftmaxOptions(1)),
            Unflatten(UnflattenOptions(1, {139, 9, 9}))
        ))),
        valueHead(register_module("valueHead", Sequential(
            Conv2d(Conv2dOptions(channels, valueHeadChannels, 1).stride(1)),
            GlobalAvgMaxPool2d(),
            Linear(LinearOptions(2 * valueHeadChannels, finalValueHeadChannels)),
            ReLU(),
            Linear(LinearOptions(finalValueHeadChannels, 4))
        ))),
        wdlProb(register_module("wdlProb", Softmax(SoftmaxOptions(1)))),
        activateDFPN(register_module("activateDFPN", Tanh())) {
        for (int i = 0; i < blocks; i++)
            if (blocks - i <= poolBlocks)
                resBlocks.push_back(register_module("resBlock_p" + std::to_string(i), Sequential(
                    BatchNorm2d(BatchNorm2dOptions(channels)),
                    ReLU(),
                    Conv2d(Conv2dOptions(channels, channels, 3).stride(1).padding(1)),
                    AutoSplittingGBS(channels, poolTrunk),
                    BatchNorm2d(BatchNorm2dOptions(channels - poolTrunk)),
                    ReLU(),
                    Conv2d(Conv2dOptions(channels - poolTrunk, channels, 3).stride(1).padding(1))
                )));
            else
                resBlocks.push_back(register_module("resBlock" + std::to_string(i), Sequential(
                    BatchNorm2d(BatchNorm2dOptions(channels)),
                    ReLU(),
                    Conv2d(Conv2dOptions(channels, channels, 3).stride(1).padding(1)),
                    BatchNorm2d(BatchNorm2dOptions(channels)),
                    ReLU(),
                    Conv2d(Conv2dOptions(channels, channels, 3).stride(1).padding(1))
                )));
    }

    void forward(torch::Tensor binput, const torch::Tensor& ninput, torch::Tensor output[3]) {
        binput = binputLayer->forward(binput) + ninputLayer->forward(ninput).unsqueeze(2).unsqueeze(3);
        for (int i = 0; i < resBlocks.size(); i++)
            binput = resBlocks[i]->forward(binput) + binput;
        output[0] = policyHead->forward(binput);
        binput = valueHead->forward(binput);
        std::vector<torch::Tensor> splitted = torch::split_with_sizes(binput, {3, 1}, 1);
        output[1] = wdlProb->forward(splitted[0]);
        output[2] = activateDFPN->forward(splitted[1]);
    }
};
TORCH_MODULE(MCTSModel);

#endif