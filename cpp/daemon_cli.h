#pragma once

#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace daemon_cli
{

template<class F> class CDeferGuard
{
public:
    CDeferGuard(F &&f) : m_f(f) {}
    ~CDeferGuard() { m_f(); }
private:
    F m_f;
};

namespace impl
{

class CReader
{
public:
    CReader(const char *ptr) : m_ptr(ptr) {}

    int ReadInt()
    {
        int val = *reinterpret_cast<const int *>(m_ptr);
        m_ptr += sizeof(val);
        return val;
    }

    std::string ReadString()
    {
        int len = ReadInt();
        std::string val;
        val.resize(len, 0);
        std::memcpy(val.data(), m_ptr, len);
        m_ptr += len;
        return val;
    }

    std::vector<std::string> ReadStringVector()
    {
        int count = ReadInt();
        std::vector<std::string> val;
        val.reserve(count);
        for (int i = 0; i < count; i++)
        {
            val.push_back(ReadString());
        }
        return val;
    }

private:
    const char *m_ptr;
};

class CWriter
{
public:
    const std::vector<char> &Data() const { return m_buf; }

    void WriteInt(int val)
    {
        m_buf.resize(m_buf.size() + sizeof(val));
        char *ptr = &*(m_buf.end() - sizeof(val));
        *reinterpret_cast<int *>(ptr) = val;
    }

    void WriteString(const char *val, int len)
    {
        WriteInt((int) len);
        for (int i = 0; i < len; i++)
        {
            m_buf.push_back(val[i]);
        }
    }

    void WriteStringArray(char **val, int count)
    {
        WriteInt(count);
        for (int i = 0; i < count; i++)
        {
            WriteString(val[i], (int) std::strlen(val[i]));
        }
    }

    void WriteEnvironArray(char **val)
    {
        if (val == nullptr)
        {
            WriteInt(0);
            return;
        }

        int count = 0;
        for (char **p = val; *p != nullptr; p++) count += 1;
        WriteInt(count);
        for (int i = 0; i < count; i++)
        {
            WriteString(val[i], (int) std::strlen(val[i]));
        }
    }

private:
    std::vector<char> m_buf;
};

std::error_code SendRequest(int sock,
                            int fds[3],
                            int argc,
                            char **argv,
                            char **environ)
{
    int ret;

    CWriter msg2;
    msg2.WriteStringArray(argv, argc);
    msg2.WriteEnvironArray(environ);

    int msg[2] = { /* version */ 1, (int) msg2.Data().size() };

    char fdbuf[CMSG_SPACE(3 * sizeof(int))];

    struct msghdr msghdr {};

    struct iovec iovec { msg, sizeof(msg) };
    msghdr.msg_iov = &iovec;
    msghdr.msg_iovlen = 1;

    msghdr.msg_control = fdbuf;
    msghdr.msg_controllen = sizeof(fdbuf);

    struct cmsghdr *cmsghdr = CMSG_FIRSTHDR(&msghdr);
    cmsghdr->cmsg_level = SOL_SOCKET;
    cmsghdr->cmsg_type = SCM_RIGHTS;
    cmsghdr->cmsg_len = CMSG_LEN(3 * sizeof(int));

    std::memcpy((int *) CMSG_DATA(cmsghdr), fds, 3 * sizeof(int));
    msghdr.msg_controllen = cmsghdr->cmsg_len;

    ret = (int) ::sendmsg(sock, &msghdr, /* flags */ 0);
    if (ret < 0) return std::make_error_code(std::errc(errno));

    ret = (int) ::send(sock, msg2.Data().data(), msg2.Data().size(), /* flags */ 0);
    if (ret < 0) return std::make_error_code(std::errc(errno));

    return {};
}

std::error_code ReceiveRequest(int sock,
                               std::array<int, 3> *fds,
                               std::vector<std::string> *argv,
                               std::vector<std::string> *environ)
{
    int ret;

    struct msghdr msghdr {};

    int msg1[2];
    struct iovec iovec { msg1, sizeof(msg1) };
    msghdr.msg_iov = &iovec;
    msghdr.msg_iovlen = 1;

    char msg_control[256];
    msghdr.msg_control = msg_control;
    msghdr.msg_controllen = sizeof(msg_control);

    ret = (int) ::recvmsg(sock, &msghdr, /* flags */ 0);
    if (ret < 0) return std::make_error_code(std::errc(errno));

    int version = msg1[0];
    assert(version == 1);

    struct cmsghdr *cmsghdr = nullptr;
    for (cmsghdr = CMSG_FIRSTHDR(&msghdr);
         cmsghdr != nullptr;
         cmsghdr = CMSG_NXTHDR(&msghdr, cmsghdr))
    {
        if (cmsghdr->cmsg_level != SOL_SOCKET) continue;
        if (cmsghdr->cmsg_type != SCM_RIGHTS) continue;
        assert(cmsghdr->cmsg_len == CMSG_LEN(3 * sizeof(int)));
        std::memcpy(fds->data(), CMSG_DATA(cmsghdr), 3 * sizeof(int));
        break;
    }

    std::vector<char> msg2buf(msg1[1]);
    ret = (int) ::recv(sock, msg2buf.data(), msg2buf.size(), /* flags */ 0);
    if (ret < 0) return std::make_error_code(std::errc(errno));

    CReader msg2(msg2buf.data());
    *argv = msg2.ReadStringVector();
    *environ = msg2.ReadStringVector();

    return {};
}

std::error_code SendResponse(int sock, int exitcode)
{
    int ret = ::send(sock, &exitcode, sizeof(exitcode), /* flags */ 0);
    if (ret < 0) return std::make_error_code(std::errc(errno));
    return {};
}

std::error_code ReceiveResponse(int sock, int *exitcode)
{
    int ret = ::recv(sock, exitcode, sizeof(*exitcode), /* flags */ 0);
    if (ret < 0) return std::make_error_code(std::errc(errno));
    return {};
}

} // namespace impl

using THandler = std::function<int(std::array<int, 3> fds,
                                   std::vector<std::string> &&argv,
                                   std::vector<std::string> &&environ)>;

const char *Getenv(char **environ, const char *name)
{
    for (; environ != nullptr; environ++)
    {
        const char *peq = std::strchr(*environ, '=');
        int cmp = std::strncmp(*environ, name, peq - *environ);
        if (cmp == 0) return peq + 1;
    }
    return nullptr;
}

const char *Getenv(const std::vector<std::string> &environ, const char *name)
{
    for (const auto &s : environ)
    {
        const char *peq = std::strchr(s.c_str(), '=');
        int cmp = std::strncmp(s.c_str(), name, peq - s.c_str());
        if (cmp == 0) return peq + 1;
    }
    return nullptr;
}

std::vector<char *> ToCstrVector(std::vector<std::string> &vec)
{
    std::vector<char *> result;
    result.reserve(vec.size() + 1);
    for (auto &s : vec) result.push_back(s.data());
    result.push_back(nullptr);
    return result;
}

std::error_code RunServer(const std::string &socketPath,
                          std::future<void> onStop,
                          THandler &&handler)
{
    int ret;

    int listenSock = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, /* protocol */ 0);
    if (listenSock < 0) return std::make_error_code(std::errc(errno));
    CDeferGuard deferCloseListenSock = [&] { ::close(listenSock); };

    struct sockaddr_un sockaddr;
    sockaddr.sun_family = AF_UNIX;
    std::memcpy(&sockaddr.sun_path, socketPath.c_str(), socketPath.size() + 1);

    ret = ::bind(listenSock, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
    if (ret < 0) return std::make_error_code(std::errc(errno));
    CDeferGuard deferUnlink = [&] { ::unlink(socketPath.c_str()); };

    ret = ::listen(listenSock, /* backlog */ 16);
    if (ret < 0) return std::make_error_code(std::errc(errno));

    std::thread { [&] {
        onStop.wait();
        ::shutdown(listenSock, SHUT_RDWR);
    } }.detach();

    while (true)
    {
        int sock = ::accept(listenSock, /* sockaddr */ nullptr, /* addrlen */ 0);
        if (sock < 0)
        {
            if (errno == EINVAL) break;
            else return std::make_error_code(std::errc(errno));
        }

        std::thread { [sock, &handler] {
            std::error_code ec;

            CDeferGuard deferCloseSock = [&] { ::close(sock); };

            std::array<int, 3> fds;
            std::vector<std::string> argv;
            std::vector<std::string> environ;
            ec = impl::ReceiveRequest(sock, &fds, &argv, &environ);
            if (ec) { std::cerr << ec.message() << std::endl; return; }

            CDeferGuard deferCloseFds = [&]
            {
                for (int i = 0; i < 3; i++) ::close(fds[i]);
            };

            int exitcode = handler(fds, std::move(argv), std::move(environ));

            ec = impl::SendResponse(sock, exitcode);
            if (ec) { std::cerr << ec.message() << std::endl; return; }
        } }.detach();
    }

    return {};
}

std::error_code RunClient(const std::string &socketPath,
                          int argc,
                          char **argv,
                          char **environ,
                          int *exitcode)
{
    int ret;
    std::error_code ec;

    int sock = ::socket(AF_UNIX, SOCK_STREAM, /* protocol */ 0);
    if (sock < 0) return std::make_error_code(std::errc(errno));
    CDeferGuard deferCloseSock = [&] { ::close(sock); };

    struct sockaddr_un sockaddr;
    sockaddr.sun_family = AF_UNIX;
    std::memcpy(&sockaddr.sun_path, socketPath.c_str(), socketPath.size() + 1);

    ret = ::connect(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
    if (ret < 0) return std::make_error_code(std::errc(errno));

    int fds[3] = { 0, 1, 2 };
    ec = impl::SendRequest(sock, fds, argc, argv, environ);
    if (ec) return ec;

    ec = impl::ReceiveResponse(sock, exitcode);
    if (ec) return ec;

    return {};
}

} // namespace daemon_cli
