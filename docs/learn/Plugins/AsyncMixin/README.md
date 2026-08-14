# Plugins/AsyncMixin

AsyncMixin 为宿主对象提供按请求顺序执行的异步步骤：资源可以乱序完成，但业务回调按队列推进；支持软引用、Primary Asset bundle、条件等待和取消。内部用宿主指针到 LoadingState 的稀疏表，宿主析构时移除状态，避免回调访问已销毁对象。

保留 StreamableHandle 不只是等回调，也维持资产 residency。所有操作假定 GameThread；捕获 `this` 的安全性来自 mixin 生命周期协议，不是 C++ lambda 天然安全。

## 面试追问

1. 为什么资源完成顺序与业务执行顺序要分离？
2. 不保存 StreamableHandle 会发生什么？
3. 宿主析构与异步回调竞态如何收敛？
4. 为什么显式 Start 比完全依赖 next-frame ticker 更可预测？

