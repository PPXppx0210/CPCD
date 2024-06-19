#include <iostream>
#include <string.h>
#include "Lexer.h"

//����token����/����
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

//�������token������ֵ�Լ����庬��
void Lexer::outputToken(ofstream& out)
{
	out << "token���� token���� �� ��" << endl;
	int len = this->result.size();
	for (int i = 0; i < len; ++i)
		out << translatetoken(this->result[i].type) << " " << this->result[i].content << " " << this->result[i].line << " " << this->result[i].position << endl;
}

//���ļ�������token
bool Lexer::analyze(ifstream& file)
{
	int line = 0;        //��¼����һ��
	char templine[1024]; //��һ��
	int cr;              //ָ��ǰ�е��ַ���λ�õ�ָ�룬��templine[0]��ʼ
	int len;             //len��¼templine��һ�еĳ���
	Status res;          //����һЩ����״̬
	Ftype chtp;          //��¼����
	while (file.getline(templine, 1024))
	{
		line++;
		len = strlen(templine);
		cr = 0;
		while (cr < len)//����ַ���ȡ
		{
			if (this->annflag == true) //��ǰ����/**/��ע����
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
			else //�������
			{
				chtp = chtype(templine[cr]);
				if (chtp == Ftype::Blank) //��ͷ��tab��ո�
					cr++;
				else if (chtp == Ftype::Number) { //��ͷ������
					res = dealnumber(templine, cr, len, line);
					if (res == Status::Fail) {
						dealerror(templine, line, cr);
						return false;
					}
				} 
				else if (chtp == Ftype::Alpha) //��ͷ����ĸ
					res = dealalpha(templine, cr, len, line);
				else if (chtp == Ftype::Symbol) //��ͷ�Ƿ���
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

//��ȡ��ǰ�ַ�����
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

//���ֿ�ͷ
Status Lexer::dealnumber(char* templine, int& cr, int len, int line)
{
	string tempstr = "";
	while (cr < len && chtype(templine[cr]) == Ftype::Number) //һֱ������ַ���ֱ����������
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
	return Status::Success;                             //����token�Ϸ����
}

//��ĸ��ͷ
Status Lexer::dealalpha(char* templine, int& cr, int len, int line)
{
	string tempstr = "";
	Ftype chtp;     
	while (cr < len) //һֱ������ַ��������������ֻ���ĸΪֹ
	{
		//Ϊ��Ч�ʴ���ԭ����whileѭ�����
		chtp = chtype(templine[cr]);
		if (chtp != Ftype::Alpha && chtp != Ftype::Number)break;
		//ѭ���ڵ�����
		tempstr += templine[cr];
		cr++;
	}
	Token tk;
	tk = search_keyword(tempstr); //�ؼ��ֻ��Ǳ�ʶ��
	if (tk != Token::id) {
		TokenInfo temp;
		temp.type = tk;
		temp.content = tempstr;
		temp.line = line;
		temp.position = cr - 1;
		this->result.push_back(temp);
	}
	else //�Ǳ�ʶ��
	{
		int index = search_identifier(tempstr);       //���ʶ������
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

//���ſ�ͷ
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
		else if (templine[cr + 1] == '/') //���˫б��ע�ͣ�ֱ�Ӱ�ָ���Ƶ���һ����ĩ
		{
			cr = len;
			return Status::Success;
		}
		else if (templine[cr + 1] == '*')
		{
			this->annflag = true; //����ע�Ϳ���
			cr += 2;
			return Status::Success;
		}
		else {//��Ϊ����
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

//token����
void Lexer::dealerror(char* templine, int line, int cr)
{
	cout << "�ʷ��������󣡴���λ�ڵ�" << line << "�е�" << cr << "���ַ��������ַ�Ϊ" << templine[cr] << endl;
	cout << "�����������ȷ��������ϴʷ������Դ����" << endl;
}

//���عؼ��ֱ�ǻ��ʶ�����
Token Lexer::search_keyword(string& s)
{
	for (vector<string>::size_type i = 0; i < this->keywordlist.size(); i++)//���������ֱ�
	{
		if (s == this->keywordlist[i]) //����鵽�ǹؼ���
		{
			return Token(int(Token::Int) + i); //��������ؼ��ֵ�����
		}
	}
	return Token::id; //��һ���ʶ�������ǹؼ���
}

//���ر�ʶ��������
int Lexer::search_identifier(string& s)
{
	vector<string>::size_type i;
	for (i = 0; i < this->idtlist.size(); i++)
	{
		if (s == this->idtlist[i]) //����鵽�Ǳ�ʶ��
		{
			return i; //���������ʶ��������
		}
	}
	this->idtlist.push_back(s);//û�鵽������
	return i; //���������ʶ��������
}