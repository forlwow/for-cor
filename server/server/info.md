服务器

    accept() ---socket---> handler(socket) (cor)

    handler(socket):
        recv
        llhttp_exec
        find Methodhandler(path, method) in router
        new context(recved request)
        Methodhandle(context(httprequtest))
        new response
        response.set(context.userdata)
        send response

    struct Router{
        string curpath          // find by parents .e.p /(/) or path(/path) or index(/index)
        map<string, *Router> children
        map<string, function<void(context)>) callback // (Method, callback)
    }

    Context:
        RequestContext  // uesd to send request
        ResponseContext // uesd to send response from recved requsetHttp

    User Interface:
        server.Get(path, function<void(context)>)
        server.Post(path, function<void(context)>)
            put callback into router