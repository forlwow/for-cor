#include "http_server.h"
#include "util.h"
#include <socketfunc_cpp20.h>
#include <yaml-cpp/null.h>

namespace server
{

static auto logger = SERVER_LOGGER_SYSTEM;

    const char request[] =
        "GET /example-path HTTP/1.0\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    const char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Date: Thu, 23 Jan 2025 12:34:56 GMT\r\n"
        "Server: Apache/2.4.41 (Ubuntu)\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: 120\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "    <title>Example Page</title>"
        "</head>"
        "<body>"
        "    <h1>Welcome to Example Page</h1>"
        "</body>"
        "</html>"
    ;

    const char body[] = {
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "    <title>Example Page</title>"
        "</head>"
        "<body>"
        "    <h1>Welcome to Example Page</h1>"
        "</body>"
        "</html>"
    };

    const char pageNotFound[] = {
        R"(
        <!DOCTYPE html>
        <html lang="zh">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>404 页面未找到</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    background-color: #f4f4f4;
                    color: #333;
                    text-align: center;
                    padding: 50px;
                }
                h1 {
                    font-size: 100px;
                    color: #e74c3c;
                }
                p {
                    font-size: 20px;
                }
                a {
                    color: #3498db;
                    text-decoration: none;
                    font-size: 18px;
                }
                a:hover {
                    text-decoration: underline;
                }
            </style>
        </head>
        <body>
            <h1>404</h1>
            <p>抱歉，您访问的页面无法找到。</p>
            <p>您可以返回 <a href="/">首页</a> 或检查URL是否正确。</p>
        </body>
        </html>
        )"
    };

void defaultCallBack(HttpContext::ptr c) {
    c->HTML(404, pageNotFound);
}

std::string HttpGmtTime() {
    time_t now = time(0);
    tm* gmt = gmtime(&now);

    // http://en.cppreference.com/w/c/chrono/strftime
    // e.g.: Sat, 22 Aug 2015 11:48:50 GMT
    //       5+   3+4+   5+   9+       3   = 29
    const char* fmt = "%a, %d %b %Y %H:%M:%S GMT";
    char tstr[30];

    strftime(tstr, sizeof(tstr), fmt, gmt);

    return tstr;
}

HttpContextResponse::HttpContextResponse(http::HttpRequest::ptr req) {
    if (!req) return;
    m_request = req;
    m_response = std::make_shared<http::HttpResponse>();
    m_response->SetVersion(m_request->GetVersion());
    m_response->SetHeaders("Data", HttpGmtTime());
}

void HttpContextResponse::JSON(http::HttpStatus s, const std::unordered_map<std::string, std::string>& j) {
    m_response->SetStatus(s);
    m_response->SetReason(http::HttpStatus2String(s));
    m_response->SetHeaders("Content-Type", "application/json");
    std::string ss;
    ss.push_back('{');
    for (auto it = j.begin(); it != j.end(); ) {
        // ss += "\"" + it->first + "\":\"" + it->second + "\"" + (++it == j.end() ? "" : ",");
        ss.push_back('\"');
        ss.append(it->first);
        ss.append("\":\"");
        ss.append(it->second);
        ss.push_back('"');
        ss.push_back(++it == j.end() ? ' ' : ',');
    }
    ss.push_back('}');
    m_response->SetBody(ss);
}

void HttpContextResponse::HTML(http::HttpStatus s, std::string_view b) {
    m_response->SetHeaders("Content-Type", "text/html; charset=UTF-8");
    m_response->SetStatus(s);
    m_response->SetReason(http::HttpStatus2String(s));
    m_response->SetBody(b);
}

void HttpContextResponse::SetHeader(const std::string& key, std::string_view value) {
    m_response->SetHeaders(key, value);
}

std::string_view HttpContextResponse::GetHeader(const std::string& key) {
    return m_request->GetHeader(key);
}

// TODO: test
// TODO: 改为异步
void UserCB(HttpContext::ptr c) {
    // c->HTML(http::HTTP_STATUS_OK, body);
    c->JSON(http::HTTP_STATUS_OK, {
        {"1", "aaa"},
        {"2", "bbb"},
        {"3", "ccc"}
    });
}

// Router
Router::Router() {
    m_head = std::make_shared<RouterNode>("/");
}

Router::~Router() = default;

bool Router::AddRoute(std::string_view method, std::string_view path, callback cb) {
    if (!isVaildPath(path)) {
        SERVER_LOG_ERROR(logger) << "AddRouter Path Error, path: " << path;
        return false;
    }
    RouterNode::ptr cur_node = m_head;
    size_t start = 1, end = 1;
    while ((end = path.find('/', start)) != std::string::npos) {
        std::string_view sub_path = path.substr(start, end - start);
        std::string path_str(sub_path);
        if (!cur_node->m_children.contains(path_str)) {
            cur_node->m_children[path_str] = std::make_shared<RouterNode>(path_str, cur_node);
        }
        cur_node = cur_node->m_children[path_str];
        start = end + 1;
    }
    std::string last_path(path.substr(start));
    auto it = cur_node->m_children.find(last_path);
    if (it == cur_node->m_children.end()) {
        cur_node->m_children[last_path] = std::make_shared<RouterNode>(last_path, cur_node);
    }
    cur_node = cur_node->m_children[last_path];
    cur_node->m_cbs[std::string(method)] = cb;
    return true;
}

void Router::SetDefaultRoute(callback cb) {
    m_default_callback = cb;
}

Router::callback Router::GetCallback(std::string_view method, std::string_view path) const {
    if (!isVaildPath(path)) {
        SERVER_LOG_ERROR(logger) << "AddRouter Path Error, path: " << path;
        return ncb();
    }
    RouterNode::ptr cur_node = m_head;
    size_t start = 1, end = 1;
    while ((end = path.find('/', start)) != std::string::npos) {
        std::string_view sub_path = path.substr(start, end - start);
        std::string path_str(sub_path);
        if (!cur_node->m_children.contains(path_str)) return ncb();
        cur_node = cur_node->m_children[path_str];
        start = end + 1;
    }
    std::string last_path(path.substr(start));
    auto it = cur_node->m_children.find(last_path);
    if (it == cur_node->m_children.end()) return ncb();
    cur_node = cur_node->m_children[last_path];
    auto cb_it = cur_node->m_cbs.find(std::string(method));
    return (cb_it == cur_node->m_cbs.end() ? ncb() : cb_it->second);
}




// TODO: 泄露
thread_local std::queue<http::HttpRequestParser_v2::ptr> *HttpServer::m_request_parsers = new std::queue<http::HttpRequestParser_v2::ptr>;
thread_local std::queue<http::HttpResponseParser_v2::ptr> *HttpServer::m_response_parsers = new std::queue<http::HttpResponseParser_v2::ptr>;


HttpServer::HttpServer(int max_thread)
    : m_router(std::make_shared<Router>()), TcpServer(max_thread)
{
    m_router->SetDefaultRoute(defaultCallBack);
}

HttpServer::~HttpServer() {
}

void HttpServer::serverV4(std::string_view IP, uint16_t port) {
    Address::ptr addr = IPv4Address::CreateAddressPtr(IP.data(), port);
    std::vector<Address::ptr> addrs = {addr};
    std::vector<Address::ptr> fails;
    if (!bind(addrs, fails )) {
        SERVER_LOG_ERROR(logger) << "Server bind failed";
        return;
    }
    auto iom = IOManager_::GetInstance();
    this->start();
    iom->start();
    iom->wait();
}

void HttpServer::POST(std::string_view path, Router::callback cb) {
    m_router->AddRoute("POST", path, cb);
}

void HttpServer::GET(std::string_view path, Router::callback cb) {
    m_router->AddRoute("GET", path, cb);
}

void HttpServer::ANY(std::string_view method, std::string_view path, Router::callback cb) {
    m_router->AddRoute(method, path, cb);
}

void HttpServer::DEFAULT(Router::callback cb) {
    m_router->SetDefaultRoute(cb);
}


void HttpServer::HandleCallback(HttpContext::ptr context) {
    auto req = context->m_request;
    Router::callback cb = m_router->GetCallback(
        http::HttpMethod2String(req->GetMethod()),
        req->GetPath()
        );
    // UserCB(context);
    cb(std::move(context));
    return;
}


Task HttpServer::handleClient(Socket::ptr client) {
    const int BUFFER_SIZE = 1024;
    char recv_buffer[BUFFER_SIZE];
    // auto recver = server::recv(client, recv_buffer, BUFFER_SIZE);
    // auto sender = server::send(client); // 在发送前会重设 开始不需要设置缓冲
    http::HttpRequestParser_v2::ptr parser;
    if (m_request_parsers->empty()) {
        parser = std::make_shared<http::HttpRequestParser_v2>();
    }
    else {
        parser = m_request_parsers->front();
        m_request_parsers->pop();
    }
    while (true) {
        // recver.reset(recv_buffer, BUFFER_SIZE);
        // 接收数据
        // int res = co_await recver;
        int res = co_await server::recv(client, recv_buffer, BUFFER_SIZE);
        if (res < 0) {
            if (res != SOCK_EAGAIN)
                SERVER_LOG_WARN(logger) << "HTTPServer recv fail err=" << Sock_Result2String(res);
            if (res == SOCK_CLOSE)
                break;
            continue;
        }
        // 数据出错则直接丢弃
        // TODO: 处理粘包
        auto err =  static_cast<llhttp_errno_t>(parser->execute(recv_buffer, res));
        if (err != HPE_OK) {
            SERVER_LOG_WARN(logger) << "HTTPServer execute fail err=" << llhttp_errno_name(err);
            parser->Reset();
            continue;
        }
        // 接收完一个数据报后
        if (parser->IsRequestFinished()) {
            // 数据处理
            // 用户回调
            HttpContext::ptr ctx = std::make_shared<HttpContextResponse>(parser->GetData());
            HandleCallback(ctx);
            std::string data = ctx->m_response->toString();
            // sender.reset(data, data.size());
            // 循环发送数据
            int wres = 0;
            do {
                // wres = co_await sender;
                wres = co_await server::send(client, data, data.size());
                if (wres < 0) {
                    SERVER_LOG_WARN(logger) << "HTTPServer send fail err=" << Sock_Result2String(wres);
                    if (wres == SOCK_CLOSE)
                        break;
                }
            } while (wres == SOCK_REMAIN_DATA);
            if (wres == SOCK_CLOSE)
                break;
            else if (wres < 0) continue;

            SERVER_LOG_DEBUG(logger) << "HttpServer send success";
            parser->Reset();
            if (parser->GetData()->GetHeader("Connection") != "keep-alive") {
                break;
            }
        }
    }

    m_request_parsers->push(parser);
    SERVER_LOG_DEBUG(logger) << "HttpServer Client close " << client->getFd();
    co_return;
}

}
