#ifndef SERVER_TEST_HTTP_H
#define SERVER_TEST_HTTP_H
#include "http_parser.h"
#include "log.h"

namespace server
{
    namespace test{
        extern Logger::ptr logger;
        char request[] =
            "GET /example-path HTTP/1.0\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36\r\n"
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
            "Accept-Language: en-US,en;q=0.5\r\n"
            "Accept-Encoding: gzip, deflate, br\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";

        char response[] =
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

        void http_request_test()
        {
            http::HttpRequestParser parser;
            size_t s = parser.execute(request, strlen(request));
            SERVER_LOG_DEBUG(logger) << "err: " << parser.GetErrstr();
            SERVER_LOG_DEBUG(logger) << "execute num: " << s;
            SERVER_LOG_DEBUG(logger) << "method: " << http::HttpMethod2String(parser.GetData()->GetMethod());
            SERVER_LOG_DEBUG(logger) << "path: " << parser.GetData()->GetPath();
            SERVER_LOG_DEBUG(logger) << "version: " << parser.GetData()->GetVersion();
            SERVER_LOG_DEBUG(logger) << "header: " << parser.GetData()->GetHeaders();
        }
        void http_response_test()
        {
            http::HttpResponseParser parser;
            size_t s = parser.execute(response, strlen(response));
            SERVER_LOG_DEBUG(logger) << "err: " << parser.GetErrstr();
            SERVER_LOG_DEBUG(logger) << "execute num: " << s;
            SERVER_LOG_DEBUG(logger) << "version: " << http::HttpVersion2String(parser.GetData()->GetVersion());
            SERVER_LOG_DEBUG(logger) << "header: " << parser.GetData()->GetHeaders();
            SERVER_LOG_DEBUG(logger) << "status: " << http::HttpStatus2String(parser.GetData()->GetStatus());
        }
        void httpv2_request_test()
        {
            http::HttpRequestParser_v2 parser;
            parser.execute(request, strlen(request));
            SERVER_LOG_DEBUG(logger) << "err: " << parser.GetErrstr();
            SERVER_LOG_DEBUG(logger) << "method: " << http::HttpMethod2String(parser.GetData()->GetMethod());
            SERVER_LOG_DEBUG(logger) << "path: " << parser.GetData()->GetPath();
            SERVER_LOG_DEBUG(logger) << "version: " << http::HttpVersion2String(parser.GetData()->GetVersion());
            SERVER_LOG_DEBUG(logger) << "header: " << parser.GetData()->GetHeaders();
        }
        void httpv2_response_test()
        {
            http::HttpResponseParser_v2 parser;
            parser.execute(response, strlen(response));
            SERVER_LOG_DEBUG(logger) << "err: " << parser.GetErrstr();
            SERVER_LOG_DEBUG(logger) << "version: " << http::HttpVersion2String(parser.GetData()->GetVersion());
            SERVER_LOG_DEBUG(logger) << "Status: " << http::HttpStatus2String(parser.GetData()->GetStatus());
            SERVER_LOG_DEBUG(logger) << "reason: " << parser.GetData()->GetReason();
            SERVER_LOG_DEBUG(logger) << "header: " << parser.GetData()->GetHeaders();
            SERVER_LOG_DEBUG(logger) << "body: " << parser.GetData()->GetBody();

            SERVER_LOG_DEBUG(logger) << "data: \n" << parser.GetData()->toString();


            parser.execute(response, strlen(response));
            SERVER_LOG_DEBUG(logger) << "err: " << parser.GetErrstr();
            SERVER_LOG_DEBUG(logger) << "version: " << http::HttpVersion2String(parser.GetData()->GetVersion());
            SERVER_LOG_DEBUG(logger) << "Status: " << http::HttpStatus2String(parser.GetData()->GetStatus());
            SERVER_LOG_DEBUG(logger) << "reason: " << parser.GetData()->GetReason();
            SERVER_LOG_DEBUG(logger) << "header: " << parser.GetData()->GetHeaders();
            SERVER_LOG_DEBUG(logger) << "body: " << parser.GetData()->GetBody();

            SERVER_LOG_DEBUG(logger) << "data: \n" << parser.GetData()->toString();
        }
    }
}

#endif
