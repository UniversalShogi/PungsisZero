#ifndef PUNGSIS_ZERO_H
#define PUNGSIS_ZERO_H

#include <torch/torch.h>

using namespace torch::nn;

class MCTSModelImpl : Module {
    public:
    Sequential conv10, policyHead, valueHead;

    MCTSModelImpl() : conv10(
        Conv2d(Conv2dOptions(126, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU(),
        Conv2d(Conv2dOptions(140, 140, 3).stride(1).padding(1)),
        BatchNorm2d(BatchNorm2dOptions(140)),
        ReLU()
    ), policyHead(
        Conv2d(Conv2dOptions(140, 139, 1).stride(1)),
        Flatten(FlattenOptions().start_dim(1)),
        Softmax(SoftmaxOptions(1)),
        Unflatten(UnflattenOptions(1, {139, 9, 9}))
    ), valueHead(
        Conv2d(Conv2dOptions(140, 1, 1).stride(1)),
        BatchNorm2d(BatchNorm2dOptions(1)),
        ReLU(),
        Flatten(FlattenOptions().start_dim(1)),
        Linear(LinearOptions(81, 64)),
        ReLU(),
        Linear(LinearOptions(64, 1)),
        Tanh()
    ) {
        this->register_module("conv10", this->conv10);
        this->register_module("policyHead", this->policyHead);
        this->register_module("valueHead", this->valueHead);
    }

    void forward(torch::Tensor input, torch::Tensor* values) {
        input = conv10->forward<torch::Tensor>(input);
        values[0] = policyHead->forward<torch::Tensor>(input);
        values[1] = valueHead->forward<torch::Tensor>(input);
    }
};
TORCH_MODULE(MCTSModel);

#endif