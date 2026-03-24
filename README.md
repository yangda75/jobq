# jobq

1. thread safe job queue

## close 
`jobq::Q::close()` 用于关闭队列。
- 关闭后，新的提交操作都会返回失败
- 关闭后，可以继续从队列中拉取剩余元素，如果为空，阻塞的拉取也会立刻返回nullopt
- 保证幂等，第一次调用后即关闭，后续所有调用无影响


