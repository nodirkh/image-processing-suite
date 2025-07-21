#include "node.hpp"

namespace ips
{

Node::Node(std::function<EXECError(Image&)> initFunc,
           std::function<EXECError(Image&)> runFunc)
    : Init(initFunc), Run(runFunc)
{
}

void Node::setFunctions(std::function<EXECError(Image&)> initFunc, std::function<EXECError(Image&)> runFunc)
{
    Init = initFunc;
    Run = runFunc;
}

EXECError Node::executeInit(Image& image)
{
    EXECError status = EXECError::EXEC_SUCCESS;
    if (Init)
        status = Init(image);
    return status;
}

EXECError Node::executeRun(Image& image)
{
    EXECError status = EXECError::EXEC_SUCCESS;
    if (Run)
        status = Run(image);
    return status;
}

}  // namespace ips
