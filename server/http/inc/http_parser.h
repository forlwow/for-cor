#ifndef SERVER_HTTP_PARSRE_H
#define SERVER_HTTP_PARSER_H

#include "http.h"
#include "http_parse/http11_parser.h"
#include "http_parse/httpclient_parser.h"
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

class HttpRequestParser{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

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

class HttpResponseParser{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

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



} // namespace http


} // namespace server


#endif