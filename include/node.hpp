#ifndef IPS_NODE_HPP
#define IPS_NODE_HPP

#include <functional>
#include "image.hpp"

namespace ips
{
enum class EXECError
{
    EXEC_SUCCESS,
    EXEC_FAIL
};

class Node
{
   public:
    std::function<EXECError(Image&)> Init;
    std::function<EXECError(Image&)> Run;

    Node() = default;

    Node(std::function<EXECError(Image&)>, std::function<EXECError(Image&)>);

    void setFunctions(std::function<EXECError(Image&)>, std::function<EXECError(Image&)>);

    EXECError executeInit(Image&);
    
    EXECError executeRun(Image&);
};
}  // namespace ips

#endif