#ifndef SERVER_HTTP_SERVER_H
#define SERVER_HTTP_SERVER_H

#include <algorithm>
#include <queue>
#include <unordered_set>
#include <utility>
#include "http_parser.h"
#include "TCP_server.h"

namespace server {

struct MiddlewareHandler {

};

struct HttpContext {
    typedef std::shared_ptr<HttpContext> ptr;
    virtual ~HttpContext() = default;
    virtual void JSON(http::HttpStatus, const std::unordered_map<std::string, std::string>&)=0;
    virtual void HTML(http::HttpStatus, std::string_view)=0;
    virtual void JSON(int, const std::unordered_map<std::string, std::string>&)=0;
    virtual void HTML(int, std::string_view)=0;
    http::HttpRequest::ptr m_request;
    http::HttpResponse::ptr m_response;
    MiddlewareHandler m_handler;
};

// 用于产生http响应的上下文
struct HttpContextResponse:public HttpContext {
    typedef std::shared_ptr<HttpContextResponse> ptr;
    // 通过Request初始化
    HttpContextResponse(http::HttpRequest::ptr req);
    void JSON(http::HttpStatus, const std::unordered_map<std::string, std::string>&) override;
    void HTML(http::HttpStatus, std::string_view) override;
    void JSON(int s, const std::unordered_map<std::string, std::string>& j) override {JSON(static_cast<http::HttpStatus>(s), j);}
    void HTML(int s, std::string_view v) override {HTML(static_cast<http::HttpStatus>(s), v);}
};

struct Router {
    typedef std::shared_ptr<Router> ptr;
    typedef void(*callback)(HttpContext::ptr ctx);
private:
    struct RouterNode {
        typedef std::shared_ptr<RouterNode> ptr;
        RouterNode(std::string cur_path, RouterNode::ptr parent = nullptr) {
            m_cur_path = std::move(cur_path);
            m_parent = std::move(parent);
        }
        std::weak_ptr<RouterNode> m_parent;
        std::string m_cur_path;
        std::unordered_map<std::string, callback> m_cbs;  // 用户回调<方法, 函数指针>
        std::unordered_map<std::string, std::shared_ptr<RouterNode>> m_children;
    };
protected:
    RouterNode::ptr m_head;    // 头结点
    callback m_no_callback = [](HttpContext::ptr){};    // 空回调 查找失败返回

public:
    Router();
    ~Router();
    // 添加路由
    bool AddRoute(std::string_view method, std::string_view path, callback cb);
    // 获取回调函数 失败返回no_callback
    callback GetCallback(std::string_view method, std::string_view path) const;
    callback ncb() const {return m_no_callback;}
};

class HttpServer: public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer();
    ~HttpServer() override ;

    // 添加路由方法 PATH callback
    void POST(std::string_view, Router::callback);
    void GET(std::string_view, Router::callback);
    void ANY(std::string_view, std::string_view, Router::callback); // METHOD PATH callback

protected:
    Task handleClient(Socket::ptr client) override;

private:
    Router::ptr m_router;
    // http报文解析器 每个连接分配一个
    // TODO: 改线程安全
    static thread_local std::queue<http::HttpRequestParser_v2::ptr> *m_request_parsers;
    static thread_local std::queue<http::HttpResponseParser_v2::ptr> *m_response_parsers;

private:
    void HandleCallback(HttpContext::ptr ctx);
};

}

#endif