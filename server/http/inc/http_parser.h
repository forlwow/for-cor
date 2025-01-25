#ifndef SERVER_HTTP_PARSRE_H
#define SERVER_HTTP_PARSER_H

#include "http.h"
#include "http_parse/http11_parser.h"
#include "http_parse/httpclient_parser.h"
#include "llhttp/llhttp.h"
#include <memory>

namespace server{

namespace http{

enum HTTP_PARSER_ERROR{
    NOERROR                 = 0b0000,
    PARSRE_ERROR            = 0b0001,
    INVALID_HTTP_VERSION    = 0b0010,
    INVALID_FIELD_LEN       = 0b0100,
    INVALID_METHOD          = 0b1000,
};

class HttpRequestParser_v1{
public:
    typedef std::shared_ptr<HttpRequestParser_v1> ptr;
    HttpRequestParser_v1();

    size_t execute(char* data, size_t len, size_t off = 0);
    int isFinished() const ;
    int GetError() const {return m_error | http_parser_has_error(&m_parser);}
    std::string GetErrstr() const;
    void SetError(int e) {m_error |= e;}

    HttpRequest::ptr GetData() const {return m_data;}
private: 
    int m_error = 0;
    HttpRequest::ptr m_data;
    mutable http_parser m_parser;
};

class HttpResponseParser_v1{
public:
    typedef std::shared_ptr<HttpResponseParser_v1> ptr;
    HttpResponseParser_v1();

    size_t execute(char* data, size_t len, size_t off = 0);
    int isFinished() const;
    int GetError() const {return m_error | httpclient_parser_has_error(&m_parser);}
    std::string GetErrstr() const;
    void SetError(int e) {m_error |= e;}
    HttpResponse::ptr GetData() const {return m_data;}
private:
    int m_error = 0;
    HttpResponse::ptr m_data;
    mutable httpclient_parser m_parser;

};

class HttpRequestParser_v2{
public:
    typedef std::shared_ptr<HttpRequestParser_v1> ptr;
    HttpRequestParser_v2();

    size_t execute(char* data, size_t len, size_t off = 0);
    int isFinished() const ;
    int GetError() const {return m_error | llhttp_get_errno(&m_parser);}
    std::string GetErrstr() const;
    void SetError(llhttp_errno_t e) {m_error = e;}

    HttpRequest::ptr GetData() const {return m_data;}
    std::string& Getbuffer() {return m_buff;}

    void Reset(HttpRequest::ptr = nullptr);
private:
    llhttp_errno_t m_error = HPE_OK;
    HttpRequest::ptr m_data;
    std::string m_buff;
    mutable llhttp_t m_parser;
    mutable llhttp_settings_t m_settings;
};

class HttpResponseParser_v2{
public:
    typedef std::shared_ptr<HttpResponseParser_v1> ptr;
    HttpResponseParser_v2();

    size_t execute(char* data, size_t len, size_t off = 0);
    int isFinished() const;
    int GetError() const {return m_error;}
    std::string GetErrstr() const;
    void SetError(llhttp_errno_t e) {m_error = e;}
    HttpResponse::ptr GetData() const {return m_data;}

    std::string& Getbuffer() {return m_buff;}
    void Reset(HttpResponse::ptr = nullptr);
private:
    llhttp_errno_t m_error = HPE_OK;
    HttpResponse::ptr m_data;
    std::string m_buff;
    mutable llhttp_t m_parser;
    mutable llhttp_settings_t m_settings;
};


    typedef HttpRequestParser_v1 HttpRequestParser;
    typedef HttpResponseParser_v1 HttpResponseParser;

} // namespace http


} // namespace server


#endif