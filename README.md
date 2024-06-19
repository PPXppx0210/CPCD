## 编译原理课程设计

#### 本项目使用C++语言实现了一个类C语言的编译器，可以提供词法分析、语法分析、符号表管理、语义分析和中间代码生成以及目标代码生成等功能。基本过程是输入类C语言源程序，首先进行词法分析得到词法分析序列，然后根据语法规则对词法分析结果进行语法分析，使用语法制导的翻译技术，在语法分析的同时管理符号表并且进行语义分析和四元式中间代码生成，最后在中间代码的基础上生成目标代码。

#### 项目框架

程序运行文件|输入输出文件
:--:|:--:
main.cpp：主程序|test.txt：类C语言测试程序
Lexer.h：词法分析器头文件|parse.txt：语法规则
Lexer.cpp：词法分析器程序|Token.txt：词法分析结果
Parsing.h：语法分析器头文件|DFA.txt：DFA项目集
Parsing.cpp：语法分析器程序|LRAnalyser.txt：LR分析表
SymbolTable.h：符号表头文件|SyntaxTree.dot：语法树图形描述
SymbolTable.cpp：符号表程序|SyntaxTree.png：语法树图
QuadCode.h：四元式代码头文件|MidCode.txt：四元式中间代码
QuadCode.cpp：四元式代码程序|BaseBlock.txt：基本块划分结果
ObjectCode.h：目标代码生成器头文件|InformationBlocks.txt：带变量信息的基本块
ObjectCode.cpp：目标代码生成器程序|ObjCode.asm：汇编语言目标代码

----
没有上传课设报告，有需要或者其他问题可以联系QQ：1522600649
