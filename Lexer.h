#ifndef LEXER_H
#define LEXER_H
#include <vector>
#include <fstream>
#include <string>
using namespace std;

enum class Token
{
	id = 0,
	Int, Void, If, Else, While, Return,
	Plus, Minus, Multiply, Divide, Equal, Equal2, Bigger, BiggerEqual, Smaller, SmallerEqual, NotEqual,
	LeftArray, RightArray, Semi, Comma, LeftAnno, RightAnno, Anno, LeftBracket, RightBracket, LeftBrace, RightBrace,
	Number,
	End,
	Unknown
};
enum class Ftype
{
	Alpha,
	Number,
	Symbol,
	Blank
};
enum class Status
{
	Success,
	Fail
};
struct TokenInfo {
	Token type;     // token 类型
	string content; // token 内容
	int line;       // 在源代码中的行号
	int position;   // 在源代码中的字符位置
};
class Lexer
{
public:
	Lexer() = default;
	string translatetoken(Token i);             //翻译token
	void outputToken(ofstream&);                //打印结果
	bool analyze(ifstream& file);               //分析源代码
	const vector<TokenInfo>& output();//返回结果
private:
	vector<string> keywordlist = { "int", "void", "if", "else", "while", "return" }; //保留字表
	vector<string> idtlist;                                                          //标识符表
	bool annflag = false;                                                            //注释标记
	vector<TokenInfo> result;                                                        //词法分析结果

	Ftype chtype(char ch);                               //查看字符是什么类型
	Status dealnumber(char* templine, int& cr, int len, int line); //处理开头是数字的词
	Status dealalpha(char* templine, int& cr, int len, int line);  //处理开头是字母的词
	Status dealsymbol(char* templine, int& cr, int len, int line); //处理开头是符号的词
	void dealerror(char* templine, int line, int cr);                    //输出错误
	Token search_keyword(string& s);                     //查询关键字表
	int search_identifier(string& s);                    //查标识符表
};

#endif