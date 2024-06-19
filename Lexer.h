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
	Token type;     // token ����
	string content; // token ����
	int line;       // ��Դ�����е��к�
	int position;   // ��Դ�����е��ַ�λ��
};
class Lexer
{
public:
	Lexer() = default;
	string translatetoken(Token i);             //����token
	void outputToken(ofstream&);                //��ӡ���
	bool analyze(ifstream& file);               //����Դ����
	const vector<TokenInfo>& output();//���ؽ��
private:
	vector<string> keywordlist = { "int", "void", "if", "else", "while", "return" }; //�����ֱ�
	vector<string> idtlist;                                                          //��ʶ����
	bool annflag = false;                                                            //ע�ͱ��
	vector<TokenInfo> result;                                                        //�ʷ��������

	Ftype chtype(char ch);                               //�鿴�ַ���ʲô����
	Status dealnumber(char* templine, int& cr, int len, int line); //����ͷ�����ֵĴ�
	Status dealalpha(char* templine, int& cr, int len, int line);  //����ͷ����ĸ�Ĵ�
	Status dealsymbol(char* templine, int& cr, int len, int line); //����ͷ�Ƿ��ŵĴ�
	void dealerror(char* templine, int line, int cr);                    //�������
	Token search_keyword(string& s);                     //��ѯ�ؼ��ֱ�
	int search_identifier(string& s);                    //���ʶ����
};

#endif