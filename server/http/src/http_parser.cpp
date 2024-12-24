#include "http_parser.h"
#include "http.h"
#include "http_parse/http11_parser.h"
#include "log.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>

namespace server{

namespace http{

static Logger::ptr s_log = SERVER_LOGGER_SYSTEM;

static const uint64_t s_http_request_buffer_size = 1 << 12;   // 请求体最大大小 4KB
static const uint64_t s_http_request_max_body_size = 1 << 26; // 请求内容最大大小 64MB


// 读取到Request Method触发的回调
void on_request_method(void *data, const char*at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = String2HttpMethod(at);

    if(m == HTTP_METHOD_INVALID){
        // TODO: set error num
        parser->SetError(INVALID_METHOD);
        return ;
    }
    parser->GetData()->SetMethod(m);
}
void on_request_uri(void *data, const char*at, size_t length){

}
void on_request_fragment(void *data, const char*at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->GetData()->SetFragment(at);
}
void on_request_path(void *data, const char*at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->GetData()->SetPath(at);
}
void on_request_query(void *data, const char*at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->GetData()->SetQuery(at);
}
void on_request_version(void *data, const char*at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    int v;
    if(strcmp(at, "HTTP/1.1") == 0){
        v = 0x11;
    }
    else if(strcmp(at, "HTTP/1.0") == 0){
        v = 0x10;
    }
    else{
        parser->SetError(INVALID_HTTP_VERSION);
        return ;
    }
    parser->GetData()->SetVersion(v);
}
void on_request_header_done(void *data, const char*at, size_t length){

}

void on_request_http_field(void *data, const char* field, size_t flen, const char *value, size_t vlen){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if (flen == 0){
        parser->SetError(INVALID_FIELD_LEN);
    }
    parser->GetData()->SetHeaders({field, flen}, {value, vlen});
}



HttpRequestParser::HttpRequestParser()
{
    m_data.reset(new HttpRequest);
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}

size_t HttpRequestParser::execute(char* data, size_t len, size_t off){
    size_t offset = http_parser_execute(&m_parser, data, len, off);
    memmove(data, (data + offset), (len- offset));
    return offset;
}

int HttpRequestParser::isFinished() const {
    return http_parser_is_finished(&m_parser);
}

std::string HttpRequestParser::GetErrstr() const {
    std::string res ;
    int err = GetError();
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append("PARSRE_ERROR");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append("INVALID_HTTP_VERSION");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append("INVALID_FIELD_LEN");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append("INVALID_METHOD");
    }
    if(res.empty()){
        return "NOERROR";
    }
    return res;  
}

void on_response_reason(void *data, const char *at, size_t lenght){
    HttpResponseParser *parser = static_cast<HttpResponseParser*>(data);
    parser->GetData()->SetReason(at);
}
void on_response_status(void *data, const char *at, size_t lenght){
    HttpResponseParser *parser = static_cast<HttpResponseParser*>(data);
    parser->GetData()->SetStatus(String2HttpStatus(at));
}
void on_response_chunk(void *data, const char *at, size_t lenght){
    HttpResponseParser *parser = static_cast<HttpResponseParser*>(data);
}
void on_response_version(void *data, const char *at, size_t lenght){
    HttpResponseParser *parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if(strcmp(at, "HTTP/1.1") == 0){
        v = 0x11;
    }
    else if (strcmp(at, "HTTP/1.0") == 0){
        v = 0x10;
    }
    else{
        parser->SetError(INVALID_HTTP_VERSION);
        return ;
    }
    parser->GetData()->SetVersion(v); 
}
void on_response_header_done(void *data, const char *at, size_t lenght){

}
void on_response_last_chunk(void *data, const char *at, size_t lenght){

}

void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen){
    HttpResponseParser *parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0){
        parser->SetError(INVALID_FIELD_LEN);
        return;
    }
    parser->GetData()->SetHeaders({field, flen}, {value, vlen});
}


HttpResponseParser::HttpResponseParser(){
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}

size_t HttpResponseParser::execute(char* data, size_t len, size_t off){
    size_t offset = httpclient_parser_execute(&m_parser, data, len, off);
    memmove(data, (data + offset), (len - offset));
    return offset;
}
  
int HttpResponseParser::isFinished() const{
    return httpclient_parser_is_finished(&m_parser);
}

std::string HttpResponseParser::GetErrstr() const {
    std::string res ;
    int err = GetError();
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":PARSRE_ERROR");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":INVALID_HTTP_VERSION");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":INVALID_FIELD_LEN");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":INVALID_METHOD");
    }
    if(res.empty()){
        return "NOERROR";
    }
    return res;  
}

} // namespace http

} // namespace server