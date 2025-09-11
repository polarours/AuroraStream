# AuroraStream 贡献指南

感谢您对 AuroraStream 项目的关注！我们欢迎各种形式的贡献，包括代码提交、文档改进、问题报告和功能建议。

## 开发环境设置

### 前提条件
- Git
- CMake 3.10+
- C++17 兼容编译器
- 相关开发库（GLFW, GLEW, OpenAL）

### 克隆仓库
```shell
git clone https://github.com/polarours/AuroraStream.git
cd AuroraStream
git submodule update --init --recursive
```

### 构建项目
```shell
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 贡献流程

### 1. 创建分支
```shell
git checkout -b feature/your-feature-name
# 或
git checkout -b fix/issue-number-description
```

### 2. 进行修改
- 遵循项目的代码风格
- 添加适当的测试
- 更新相关文档

### 3. 提交更改
```shell
git add .
git commit -m "feat: 添加新功能描述"
# 或
git commit -m "fix: 修复问题描述"
```

### 4. 推送到远程
```shell
git push origin feature/your-feature-name
```

### 5. 创建 Pull Request
- 在 GitHub 上创建 Pull Request
- 提供清晰的描述和相关的 Issue 编号
- 等待代码审查

## 代码规范

### 代码风格
- 使用 4 空格缩进
- 遵循 Google C++ Style Guide
- 使用 descriptive 的变量和函数名

### 提交信息规范
使用 Conventional Commits 格式：
- `feat:` 新功能
- `fix:` bug 修复
- `docs:` 文档更新
- `style:` 代码格式调整
- `refactor:` 代码重构
- `test:` 测试相关
- `chore:` 构建过程或辅助工具变动

### 示例
```shell
git commit -m "feat: 添加视频字幕支持"
git commit -m "fix: 修复音频播放卡顿问题"
git commit -m "docs: 更新安装指南"
```

## 测试要求

### 单元测试
- 为新功能添加相应的单元测试
- 确保所有测试通过
- 测试覆盖率不应降低

### 集成测试
- 测试跨平台兼容性
- 验证硬件加速功能
- 测试不同媒体格式的支持

## 文档要求

### 代码注释
- 为公共接口添加 Doxygen 风格注释
- 解释复杂的算法逻辑
- 注明重要的设计决策

### 用户文档
- 更新相应的用户手册
- 添加新功能的用法示例
- 保持文档与代码同步

## 审查流程

### 代码审查标准
- 代码符合项目风格指南
- 功能实现正确且高效
- 包含适当的测试用例
- 文档更新完整

### 审查反馈
- 审查意见会在 2-3 个工作日内提供
- 可能需要多次迭代修改
- 最终由项目维护者合并

## 问题报告

### 提交 Issue
- 使用问题模板
- 提供详细的重现步骤
- 包含系统环境和版本信息
- 附加相关的日志文件

### Bug 报告模板
```markdown
## 问题描述
[清晰描述问题]

## 重现步骤
1. 
2. 
3. 

## 期望行为
[描述期望的结果]

## 实际行为
[描述实际的结果]

## 环境信息
- 操作系统: 
- 版本: 
- 硬件信息:

## 附加信息
[日志、截图等]
```

## 功能建议

### 提出新功能
- 描述使用场景和需求
- 提供初步的设计思路
- 讨论可能的实现方案

### 功能请求模板
```markdown
## 功能描述
[详细描述需求功能]

## 使用场景
[说明在什么情况下需要这个功能]

## 建议实现
[提供初步的实现思路]

## 附加价值
[说明这个功能带来的好处]
```

## 行为准则

### 社区准则
- 尊重所有贡献者
- 建设性的技术讨论
- 欢迎新手提问和学习

### 沟通渠道
- GitHub Issues: 问题讨论和功能建议
- Pull Requests: 代码贡献和审查
- Wiki: 项目文档和指南

## 许可证

所有贡献都将遵循项目的 MIT 许可证。提交代码即表示您同意按此许可证授权您的贡献。

## 获取帮助

- 查看 [开发文档](development/)
- 阅读 [架构设计](architecture/)
- 在 GitHub Issues 中提问
- 联系维护团队

感谢您的贡献！🚀