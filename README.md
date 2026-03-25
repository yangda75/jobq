# jobq

## jobq::Q 
线程安全的任务队列。
### 生命周期
```mermaid
graph Q.Lifecycle;
    OPEN-->CLOSING;
    CLOSING-->CLOSED;
```
从构造开始，处于`OPEN`状态；调用`close`后，进入`CLOSING`状态；`CLOSING`状态中，不再接受push，pop到队列为空时，进入`CLOSED`状态。

### close 
`jobq::Q::close()` 用于关闭队列。
- Q: 是幂等的吗？A: 是的。多次调用，效果一致
- Q: 调用close时，是否会唤醒所有等待的线程？A: 会。`popOne` 和 `popOneFor` 都只
在队列为空时等待，`close`后，队列会永远为空了，因此不需要再等待。
- Q: 已经入队的任务，在`close`后，还能使用吗？A: 可以，在`close`后，如果队列中还有元素，`pop` 和 `popOneFor`都能继续获取。
- Q: 由于队列为空，正在等待的 `pop` 操作，在`close`时会怎么样？A: 会结束等待，返回std::nullopt。
- Q: 限时的 `popOnFor` 操作，在 `close`时会怎么样？A:会立刻结束等待，返回std::nullopt。
- Q: 生产者会和 `close`有竞争吗？A: 不会，`close` 的同时不能`push`。

### FIFO
- Q: MPMC 场景下的顺序保证？A: 全局顺序，global ordering。即，严格按照先入队先出队
的顺序，不论执行操作的是哪个线程。
