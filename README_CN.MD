Skylark Editor - 开源, 小巧, 绿色, 秒开
==================================================================================================
**功能 :**
- 支持40多种常用编程语言的语法高亮.
- 支持50多个国家语言编码之间的相互转换.
- 支持用户自添加语法高亮配置文件.
- 支持sql与lua脚本直接运行.
- 支持括号补全,关键字补全与函数原型提示.
- 支持代码片段补全与可视化代码片段管理.
- 支持json解析与格式化, javascript压缩与格式化, clang-format格式化.
- 支持sumatrapdf插件, 可直接在skylark打开pdf文档和epub等格式电子书.
- 内置16进制编辑器, 大文件秒开.
- 内置文件管理器与sftp远程管理器.
- 内置sqlite3/mysql/PostgreSQL/redis数据库客户端.
- 内置二维码转换, 简繁转换, 半角全角符号转换功能.
- 内置常用密钥算法的加密与解密, 支持图片转base64.
- 强大的文本搜索, 二进制搜索与文件搜索功能.
- 可视化主题定制, 简洁明了的一级菜单操作方式, 支持操作系统暗黑主题模式.
- 同时提供lua脚本接口, 您不必精通lua编程, 以配置文件的方式即可定制各个模块.
- 纯绿色, 不写注册表, 不注入资源管理器, 不在安装文件位置之外写入任何东西.
- 源码全部使用c winsdk编写,可在linux/wine上无缝运行.

**注意 :**
- 对于文本文件的16进制编辑,默认显示的是其utf-8编码, 因为这样可以更多的显示多字节, 
  如果想显示原文件的16进制编码, 把标签页拖出编辑器即可.
- clang-format支持全局配置文件, 可以把__clang-format或.clang-format放入conf目录.
- 使用预设动作运行文件时, 可使用预定义宏, example_build.bat展示了内置的宏定义
- 用户配置文件会自动生成, 有时候可能会出现不兼容旧版本, 删除旧配置文件, 然后重启skylark.