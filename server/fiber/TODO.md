Fiber执行，
遇到第一个IO操作，
将Fiber加入IOManager队列(由Fiber或IOManager完成)，
并设置Event，
完成IO后清除对应Event，
后续IO直接更改IOManager中Event。


错误处理