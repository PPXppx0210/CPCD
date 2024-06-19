#include <iostream>
#include <string.h>
#include "Lexer.h"

//返回token类型/含义
string Lexer::translatetoken(Token i)
{
	switch (int(i))
	{
		case (int)Token::id:
			return "id";
		case (int)Token::Int:
			return "Int";
		case (int)Token::Void:
			return "Void";
		case (int)Token::If:
			return "If";
		case (int)Token::Else:
			return "Else";
		case (int)Token::While:
			return "While";
		case (int)Token::Return:
			return "Return";
		case (int)Token::Plus:
			return "Plus";
		case (int)Token::Minus:
			return "Minus";
		case (int)Token::Multiply:
			return "Multiply";
		case (int)Token::Divide:
			return "Divide";
		case (int)Token::Equal:
			return "Equal";
		case (int)Token::Equal2:
			return "Equal2";
		case (int)Token::Bigger:
			return "Bigger";
		case (int)Token::BiggerEqual:
			return "BiggerEqual";
		case (int)Token::Smaller:
			return "Smaller";
		case (int)Token::SmallerEqual:
			return "SmallerEqual";
		case (int)Token::NotEqual:
			return "NotEqual";
		case (int)Token::Semi:
			return "Semi";
		case (int)Token::Comma:
			return "Comma";
		case (int)Token::LeftAnno:
			return "LeftAnno";
		case (int)Token::RightAnno:
			return "RightAnno";
		case (int)Token::Anno:
			return "Anno";
		case (int)Token::LeftBracket:
			return "LeftBracket";
		case (int)Token::RightBracket:
			return "RightBracket";
		case (int)Token::LeftBrace:
			return "LeftBrace";
		case (int)Token::RightBrace:
			return "RightBrace";
		case (int)Token::Number:
			return "Number";
		case (int)Token::End:
			return "End";
		case (int)Token::LeftArray:
			return "LeftArray";
		case (int)Token::RightArray:
			return "RightArray";
		default:
			return "Unknown";
	}
	return "Unknown";
}

//输出所有token的类型值以及具体含义
void Lexer::outputToken(ofstream& out)
{
	out << "token类型 token内容 行 列" << endl;
	int len = this->result.size();
	for (int i = 0; i < len; ++i)
		out << translatetoken(this->result[i].type) << " " << this->result[i].content << " " << this->result[i].line << " " << this->result[i].position << endl;
}

//读文件并分析token
bool Lexer::analyze(ifstream& file)
{
	int line = 0;        //记录是哪一行
	char templine[1024]; //存一行
	int cr;              //指向当前行的字符的位置的指针，从templine[0]开始
	int len;             //len记录templine中一行的长度
	Status res;          //接受一些返回状态
	Ftype chtp;          //记录类型
	while (file.getline(templine, 1024))
	{
		line++;
		len = strlen(templine);
		cr = 0;
		while (cr < len)//逐个字符读取
		{
			if (this->annflag == true) //当前处在/**/的注释中
			{
				while (cr < len)
				{
					if (templine[cr] == '*' && cr + 1 < len && templine[cr + 1] == '/')
					{
						cr += 2;
						this->annflag = false;
						break;
					}
					cr++;
				}
			}
			else //正常情况
			{
				chtp = chtype(templine[cr]);
				if (chtp == Ftype::Blank) //开头是tab或空格
					cr++;
				else if (chtp == Ftype::Number) { //开头是数字
					res = dealnumber(templine, cr, len, line);
					if (res == Status::Fail) {
						dealerror(templine, line, cr);
						return false;
					}
				} 
				else if (chtp == Ftype::Alpha) //开头是字母
					res = dealalpha(templine, cr, len, line);
				else if (chtp == Ftype::Symbol) //开头是符号
				{
					res = dealsymbol(templine, cr, len, line);
					if (res == Status::Fail) {
						dealerror(templine, line, cr);
						return false;
					}
				}
				else {
					dealerror(templine, line, cr);
					return false;
				}
			}
		}
	}
	return true;
}

const vector<TokenInfo>& Lexer::output()
{
	return this->result;
}

//获取当前字符类型
Ftype Lexer::chtype(char ch)
{
	if (ch == ' ' || ch == '\t')
		return Ftype::Blank;
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
		return Ftype::Alpha;
	else if (ch >= '0' && ch <= '9')
		return Ftype::Number;
	else
		return Ftype::Symbol;
}

//数字开头
Status Lexer::dealnumber(char* templine, int& cr, int len, int line)
{
	string tempstr = "";
	while (cr < len && chtype(templine[cr]) == Ftype::Number) //一直往后读字符，直到不是数字
	{
		tempstr += templine[cr];
		cr++;
	}
	if(chtype(templine[cr]) == Ftype::Alpha)return Status::Fail;
	TokenInfo temp;
	temp.type = Token::Number;
	temp.content = tempstr;
	temp.line = line;
	temp.position = cr - 1;
	this->result.push_back(temp);
	return Status::Success;                             //返回token合法标记
}

//字母开头
Status Lexer::dealalpha(char* templine, int& cr, int len, int line)
{
	string tempstr = "";
	Ftype chtp;     
	while (cr < len) //一直往后读字符，读到不是数字或字母为止
	{
		//为了效率代替原来的while循环语句
		chtp = chtype(templine[cr]);
		if (chtp != Ftype::Alpha && chtp != Ftype::Number)break;
		//循环内的内容
		tempstr += templine[cr];
		cr++;
	}
	Token tk;
	tk = search_keyword(tempstr); //关键字还是标识符
	if (tk != Token::id) {
		TokenInfo temp;
		temp.type = tk;
		temp.content = tempstr;
		temp.line = line;
		temp.position = cr - 1;
		this->result.push_back(temp);
	}
	else //是标识符
	{
		int index = search_identifier(tempstr);       //查标识符索引
		//this->resultlt.push_back({ Token::Id, to_string(index) }); 
		TokenInfo temp;
		temp.type = Token::id;
		temp.content = tempstr;
		temp.line = line;
		temp.position = cr - 1;
		this->result.push_back(temp);
	}
	return Status::Success;
}

//符号开头
Status Lexer::dealsymbol(char* templine, int& cr, int len, int line)
{
	if (templine[cr] == '+')
	{
		this->result.push_back({ Token::Plus, "+", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '-')
	{
		this->result.push_back({ Token::Minus, "-", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '*')
	{
		this->result.push_back({ Token::Multiply, "*", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '/')
	{
		if (cr + 1 >= len)return Status::Fail;
		else if (templine[cr + 1] == '/') //变成双斜杠注释，直接把指针移到这一行最末
		{
			cr = len;
			return Status::Success;
		}
		else if (templine[cr + 1] == '*')
		{
			this->annflag = true; //开启注释开关
			cr += 2;
			return Status::Success;
		}
		else {//视为除号
			this->result.push_back({ Token::Divide, "/", line, cr });
			cr++;
			return Status::Success;
		}
	}
	else if (templine[cr] == '[')
	{
		this->result.push_back({ Token::LeftArray, "[", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == ']')
	{
		this->result.push_back({ Token::RightArray, "]", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '=')
	{
		if (cr + 1 < len && templine[cr + 1] == '=')
		{
			this->result.push_back({ Token::Equal2, "=", line, cr });
			cr += 2;
			return Status::Success;
		}
		else
		{
			this->result.push_back({ Token::Equal, "=", line, cr });
			cr++;
			return Status::Success;
		}
	}
	else if (templine[cr] == '>')
	{
		if (cr + 1 < len && templine[cr + 1] == '=')
		{
			this->result.push_back({ Token::BiggerEqual, ">=", line, cr });
			cr += 2;
			return Status::Success;
		}
		else
		{
			this->result.push_back({ Token::Bigger, ">", line, cr });
			cr++;
			return Status::Success;
		}
	}
	else if (templine[cr] == '<')
	{
		if (cr + 1 < len && templine[cr + 1] == '=')
		{
			this->result.push_back({ Token::SmallerEqual, "<=", line, cr });
			cr += 2;
			return Status::Success;
		}
		else
		{
			this->result.push_back({ Token::Smaller, "<", line, cr });
			cr++;
			return Status::Success;
		}
	}
	else if (templine[cr] == '!')
	{
		if (cr + 1 < len && templine[cr + 1] == '=')
		{
			this->result.push_back({ Token::NotEqual, "!=", line, cr });
			cr += 2;
			return Status::Success;
		}
		else return Status::Fail;
	}
	else if (templine[cr] == ',')
	{
		this->result.push_back({ Token::Comma, ",", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == ';')
	{
		this->result.push_back({ Token::Semi, ";", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '(')
	{
		this->result.push_back({ Token::LeftBracket, "(", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == ')')
	{
		this->result.push_back({ Token::RightBracket, ")", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '{')
	{
		this->result.push_back({ Token::LeftBrace, "{", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '}')
	{

		this->result.push_back({ Token::RightBrace, "}", line, cr });
		cr++;
		return Status::Success;
	}
	else if (templine[cr] == '#')
	{
		this->result.push_back({ Token::End, "#", line, cr });
		cr++;
		return Status::Success;
	}
	else return Status::Fail;
}

//token错误
void Lexer::dealerror(char* templine, int line, int cr)
{
	cout << "词法分析错误！错误位于第" << line << "行第" << cr << "个字符，错误字符为" << templine[cr] << endl;
	cout << "程序结束，请确保输入符合词法规则的源程序！" << endl;
}

//返回关键字标记或标识符标记
Token Lexer::search_keyword(string& s)
{
	for (vector<string>::size_type i = 0; i < this->keywordlist.size(); i++)//遍历保留字表
	{
		if (s == this->keywordlist[i]) //如果查到是关键字
		{
			return Token(int(Token::Int) + i); //返回这个关键字的类别号
		}
	}
	return Token::id; //是一般标识符而不是关键字
}

//返回标识符的索引
int Lexer::search_identifier(string& s)
{
	vector<string>::size_type i;
	for (i = 0; i < this->idtlist.size(); i++)
	{
		if (s == this->idtlist[i]) //如果查到是标识符
		{
			return i; //返回这个标识符的索引
		}
	}
	this->idtlist.push_back(s);//没查到，新增
	return i; //返回这个标识符的索引
}