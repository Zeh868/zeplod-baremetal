# 单元测试覆盖率基线

## 范围

`bm_event`、`bm_mempool`、`bm_channel` 及混合域模块的 native_sim 单元测试。

## 生成（GCC/Clang + gcov）

```bash
cmake -B build-cov -DBM_ENABLE_CHANNEL=ON -DBM_ENABLE_HRT=ON \
  -DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
cmake --build build-cov
ctest --test-dir build-cov
```

## 说明

Windows/MSVC 构建以功能测试通过为回归基线；覆盖率报告在 CI Linux 矩阵中可选启用。
