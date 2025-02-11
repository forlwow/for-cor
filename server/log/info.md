                                                                                                   |-----FormatItems(string, name, ...)
                                                         |--------StdAppender--------Formatter-----
                                                         |
                                                         |--------FileAppender-------...
                                                         |--------...
                              --------Logger(System)------
                              |
LogManager(Singleton) ------- |
                              |
                              -------- Logger(custom)-----...

Config:
***
    system:
    - 
        fmt: "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
        out: std
        level: DEBUG
    std:
    - 
        fmt: "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
        out: std
        level: DEBUG
    file:
    - 
        fmt: "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
        out: file
        level: DEBUG
        filename: log.txt
***


TODO: 同时输出到多个Appender

输出过程：
    define的SERVER_LOG_LEVEL创建一个LogWrap和含有默认数据的LogEvent，
    LogWrap在出define作用域后销毁，销毁时找到他的成员Event的Logger->Appender->Formatter->输出