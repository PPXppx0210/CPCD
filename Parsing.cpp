#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>
#include "Parsing.h"
using namespace std;

proc_symbolTable* beforeSymbolTable;

syntaxTreeNode::syntaxTreeNode(const TokenInfo& rhs) : index(0), parent(-1), productions(-1), type(0), inTree(false)
{
	this->type = int(rhs.type);
	this->val = rhs.content;
	this->line = rhs.line;
	this->position = rhs.position;
}

syntaxTreeNode::syntaxTreeNode() : index(0), parent(-1), productions(-1), type(0), inTree(false), quad(0), stype(symbolType::Unknown)
{
}

//���룬��������
symbolTableIndex Parsing::insertSymbol(symbolItem insrt)
{
	if (!this->symbol2Index.count(insrt)) // map�в�����
	{
		symbolTable.push_back(insrt);
		symbol2Index[insrt] = symbolTable.size() - 1;
		Index2symbol[symbolTable.size() - 1] = insrt;
	}
	return symbol2Index[insrt];
}

//��ʼ���ս������
void Parsing::initTerminalSymbol()
{
	insertSymbol("$ID");
	insertSymbol("$int");
	insertSymbol("$void");
	insertSymbol("$if");
	insertSymbol("$else");
	insertSymbol("$while");
	insertSymbol("$return");
	insertSymbol("$+");
	insertSymbol("$-");
	insertSymbol("$*");
	insertSymbol("$/");
	insertSymbol("$=");
	insertSymbol("$==");
	insertSymbol("$>");
	insertSymbol("$>=");
	insertSymbol("$<");
	insertSymbol("$<=");
	insertSymbol("$!=");
	insertSymbol("$LeftArray");
	insertSymbol("$RightArray");
	insertSymbol("$;");
	insertSymbol("$,");
	insertSymbol("$LeftAnno");
	insertSymbol("$RightAnno");
	insertSymbol("$Anno");
	insertSymbol("$(");
	insertSymbol("$)");
	insertSymbol("${");
	insertSymbol("$}");
	insertSymbol("$number");
	insertSymbol("$end");
	insertSymbol("$empty");
	insertSymbol("$Program0");
	insertSymbol("$Program");
	this->terminalSymbolMax = symbolTable.size() - 1 - 2;//�ս���������
	this->startIndex = this->symbol2Index["$Program0"];//��ʼ��������
	this->emptyIndex = this->symbol2Index["$empty"];//�շ�������
}

//��ʼ�� symbolTable��terminalSymbolMax��startIndexsymbol2Index��syntaxTable
void Parsing::initSymbolTable(ifstream& infile)
{
	initTerminalSymbol();
	int line = 0;
	char templine[1024]; //��һ��
	string tmpstr;
	while (infile.getline(templine, 1024))//���ж�ȡ�﷨�ļ�
	{
		line++;
		syntaxTableItem tmpSyntax;
		stringstream ss(templine);
		if (ss >> tmpstr)//ÿ�ζ�һ��string����
		{
			if (!((tmpstr[0] == '<' && tmpstr.back() == '>') || tmpstr[0] == '$'))
				cout << "�﷨�������: "<< "�﷨�����" << line << "����಻�Ƿ��ս��" << tmpstr << endl;
			tmpSyntax.lhs = insertSymbol(tmpstr);
			ss >> tmpstr;
			if (tmpstr != "::=")
				cout << "�﷨�������: "<< "�﷨�����" << line << "��δ�ҵ���ֵ��(::=)����Ϊ" << tmpstr << endl;
			while (ss >> tmpstr) tmpSyntax.rhs.push_back(insertSymbol(tmpstr));
			this->syntaxTable.push_back(tmpSyntax);
		}
	}
	this->syntaxTable.push_back({ this->startIndex, {this->symbol2Index["$Program"]} }); //���S' ==> S
	searchSyntaxByLhs = vector<set<syntaxTableIndex>>(symbolTable.size());
	for (int i = 0; i < syntaxTable.size(); i++)searchSyntaxByLhs[syntaxTable[i].lhs].insert(i);
}

//��ÿ�����ŵ� FIRST ��
void Parsing::initFirstTable()
{
	this->firstTable = vector<firstTableItem>(symbolTable.size());
	//�ս����FIRST��Ϊ������
	for (int i = 0; i <= this->terminalSymbolMax; i++)this->firstTable[i].insert(i);
	//���ս��FIRST���������ʽ�����
	for (int i = this->terminalSymbolMax + 1; i < this->symbolTable.size(); i++)
	{
		for (auto syntaxIndexTmp : this->searchSyntaxByLhs[i])//����ÿһ����صĲ���ʽ
		{
			//����ʽ�Ҳ�����Ϊ�ս������ֱ�Ӽ���
			symbolTableIndex symbolTmp = this->syntaxTable[syntaxIndexTmp].rhs[0];
			if (symbolTmp < this->terminalSymbolMax)this->firstTable[i].insert(symbolTmp); 
		}
	}
	//���ս��FIRST������
	bool inc = false;
	do
	{
		inc = false;
		for (int i = this->terminalSymbolMax + 1; i < this->symbolTable.size(); i++) //����ÿһ�����ս����
		{
			int cntTmp = this->firstTable[i].size();
			for (auto syntaxIndexTmp : this->searchSyntaxByLhs[i]) //����ÿһ����صĲ���ʽ
			{
				const vector<symbolTableIndex>& rhsTmp = this->syntaxTable[syntaxIndexTmp].rhs;
				for (int rhsIndex = 0; rhsIndex < rhsTmp.size(); rhsIndex++) //�����ò���ʽ����
				{
					const firstTableItem& firstSymbolSet = this->firstTable[rhsTmp[rhsIndex]];
					if (rhsTmp[rhsIndex] <= this->terminalSymbolMax) //��ǰ����Ϊ�ս��
					{
						this->firstTable[i].insert(rhsTmp[rhsIndex]);
						break;
					}
					bool haveEmpty = this->firstTable[i].count(this->emptyIndex);
					//����ǰ���ŵ� FIRST ���е�����Ԫ�ز��뵽Ŀ�� FIRST ����
					this->firstTable[i].insert(firstSymbolSet.cbegin(), firstSymbolSet.cend());
					//���������� �� ��ǰ���ŵ� FIRST ���к��գ������һ������Ŀշ�����ȥ��
					if (!haveEmpty && firstSymbolSet.count(this->emptyIndex))
						this->firstTable[i].erase(this->emptyIndex);
					if (!firstSymbolSet.count(this->emptyIndex)) break;//������������������һ���ַ�
					if (rhsIndex == rhsTmp.size() - 1) //�������ʽ�Ҳ�ȫΪ�գ����ܼ���շ���
						this->firstTable[i].insert(this->emptyIndex);
				}
			}
			if (this->firstTable[i].size() > cntTmp)inc = true;//�Ƿ�����
		}
	} while (inc); //ֱ����������
}

 //������ӵ�FIRST��
set<symbolTableIndex> Parsing::firstForPhrase(vector<symbolTableIndex> rhsTmp)
{
	set<symbolTableIndex> res;
	for (int rhsIndex = 0; rhsIndex < rhsTmp.size(); rhsIndex++) //����ÿһ���ַ�
	{
		const firstTableItem& firstSymbolSet = this->firstTable[rhsTmp[rhsIndex]];
		if (rhsTmp[rhsIndex] <= this->terminalSymbolMax) //��ǰ�ַ�Ϊ�ս��
		{
			res.insert(rhsTmp[rhsIndex]);
			break;
		}
		bool haveEmpty = res.count(this->emptyIndex);
		res.insert(firstSymbolSet.cbegin(), firstSymbolSet.cend());
		if (!haveEmpty && firstSymbolSet.count(this->emptyIndex))
			res.erase(this->emptyIndex);
		if (!firstSymbolSet.count(this->emptyIndex)) //������
			break;
		if (rhsIndex == rhsTmp.size() - 1) //���ÿһ���ַ���FIRST�������գ��ż���շ���
			res.insert(this->emptyIndex);
	}
	return res;
}

//����<�����ڱȽ�DFA_item
bool operator<(const DFA_item& A, const DFA_item& B)
{
	if (A.lhs < B.lhs)
		return true;
	else if (A.lhs > B.lhs)
		return false;

	if (A.rhs.size() < B.rhs.size())
		return true;
	else if (A.rhs.size() > B.rhs.size())
		return false;
	else
	{
		int i;
		int mm = A.rhs.size();
		for (i = 0; i < mm; i++)
		{
			if (A.rhs[i] < B.rhs[i])
				return true;
			else if (A.rhs[i] > B.rhs[i])
				return false;
		}
	}

	if (A.pos < B.pos)
		return true;
	else if (A.pos > B.pos)
		return false;

	if (A.forecast < B.forecast)
		return true;
	else if (A.forecast > B.forecast)
		return false;

	return false;
}

 //����==�����ڱȽ�DFA_item
bool operator==(const DFA_item& A, const DFA_item& B)
{
	if (A.lhs == B.lhs && A.rhs == B.rhs && A.pos == B.pos && A.forecast == B.forecast)
		return true;
	else
		return false;
}

//��״̬���ϵıհ�
pair<int, bool> Parsing::createClosure(DFA_status& sta)
{
	set<int> tempfirst;
	vector<symbolTableIndex> restsentence;
	stack<DFA_item> sd;
	DFA_item temptop, tempd;
	//�Ȱ�������Ŀ��ջ
	for (auto it = sta.begin(); it != sta.end(); it++)sd.push(*it);
	while (!sd.empty())
	{
		temptop = sd.top();
		sd.pop();
		if (temptop.pos < temptop.rhs.size() && temptop.rhs[temptop.pos] > terminalSymbolMax) //������Ƿ��ս��
		{
			restsentence.clear();
			for (int i = temptop.pos + 1; i < temptop.rhs.size(); i++) //��ȡ��Ҫ��first�������
				restsentence.push_back(temptop.rhs[i]);
			restsentence.push_back(temptop.forecast);
			tempfirst = firstForPhrase(restsentence); //���ַ���restsentence��FIRST��
			for (auto it = tempfirst.begin(); it != tempfirst.end(); it++)//����FIRST����
			{
				for (auto it2 = searchSyntaxByLhs[temptop.rhs[temptop.pos]].begin(); it2 != searchSyntaxByLhs[temptop.rhs[temptop.pos]].end(); it2++)
				{
					tempd.lhs = temptop.rhs[temptop.pos];
					tempd.rhs = syntaxTable[*it2].rhs;
					tempd.pos = 0;
					tempd.forecast = *it;
					if (sta.insert(tempd).second == true)sd.push(tempd); 
				}
			}
		}
	}
	//����Ƿ�����״̬
	for (int i = 0; i < DFA.size(); i++)
		if (DFA[i] == sta) return pair<int, bool>(i, false);
	//����״̬�������DFA״̬����
	DFA.push_back(sta);
	return pair<int, bool>(DFA.size() - 1, true);
}

//��DFA��Ŀ������LR������
void Parsing::initAnalyseTable()
{
	DFA_status temps; //DFA״̬����
	DFA_item tempd;
	pair<int, bool> gt;
	int statusno;
	stack<int> si;
	//���ֶ������ʼ״̬0
	//��һ��S'->.S,#
	tempd.lhs = symbol2Index["$Program0"];
	tempd.rhs.push_back(symbol2Index["$Program"]);
	tempd.pos = 0;
	tempd.forecast = symbol2Index["$end"];
	temps.insert(tempd);
	createClosure(temps); //����0����Ŀ��
	si.push(0); //��״̬0��ջ
	//��ʼ�Ƶ�ʣ��״̬
	set<int> transflag; //��¼����Щ���ſ�������ת��
	analyseTable.push_back(vector<analyseTableItem>(symbolTable.size(), pair<char, int>('\0', -1)));//��ʼ��������
	while (!si.empty())
	{
		statusno = si.top();
		// DFA_status& DFA[statusno] = DFA[statusno];
		si.pop();
		transflag.clear();
		for (auto it = DFA[statusno].begin(); it != DFA[statusno].end(); it++) //�ҵ�������������п���ת�Ƶ��ַ�
		{
			if ((*it).pos < (*it).rhs.size())
				transflag.insert((*it).rhs[(*it).pos]);
		}
		for (auto it = transflag.begin(); it != transflag.end(); it++) //����ÿ��������ת�Ƶ��ַ������ƽ�״̬
		{
			if (*it == emptyIndex) //���⴦������empty������
			{
				;
			}
			else //����ת��дת�Ʊ�
			{
				temps.clear();
				for (auto it2 = DFA[statusno].begin(); it2 != DFA[statusno].end(); it2++) //����ÿһ�����
				{
					if ((*it2).pos < (*it2).rhs.size() && (*it2).rhs[(*it2).pos] == *it)//����ת��
					{
						tempd.lhs = (*it2).lhs;
						tempd.rhs = (*it2).rhs;
						tempd.pos = (*it2).pos + 1;//ֻ��λ����ǰһ��
						tempd.forecast = (*it2).forecast;
						temps.insert(tempd);//����Ŀ������Ŀ��
					}
				}
				gt = createClosure(temps);//�հ�����
				analyseTable[statusno][*it] = pair<char, int>('s', gt.first);//���·������ƽ�
				if (gt.second == true) //���µ�״̬/��Ŀ��
				{
					si.push(gt.first);
					analyseTable.push_back(vector<analyseTableItem>(symbolTable.size(), pair<char, int>('\0', -1)));
				}
			}
		}
		for (auto it = DFA[statusno].begin(); it != DFA[statusno].end(); it++) //�ҹ�Լ״̬
		{
			if ((*it).pos >= (*it).rhs.size() || (*it).rhs[0] == emptyIndex)//�ɹ�Լ����
			{
				int synno;
				//�ҵ���Լ�ķ�����ʽ
				for (auto it2 = searchSyntaxByLhs[(*it).lhs].begin(); it2 != searchSyntaxByLhs[(*it).lhs].end(); it2++)
				{
					if (syntaxTable[*it2].rhs == (*it).rhs)
					{
						synno = *it2;
						break;
					}
				}
				//���Լ��
				if (analyseTable[statusno][(*it).forecast].first != '\0')cout << "wrong" << endl;//�ñ����Ѿ����
				if ((*it).lhs == symbol2Index["$Program0"] && (*it).rhs[0] == symbol2Index["$Program"] && (*it).forecast == symbol2Index["$end"])
					analyseTable[statusno][(*it).forecast] = pair<char, int>('a', -1);//����״̬
				else
					analyseTable[statusno][(*it).forecast] = pair<char, int>('r', synno);//һ���Լ״̬
			}
		}
	}
}

//���dfa��Ŀ��
void Parsing::outputDFA(ofstream& out)
{
	for (int i = 0; i < DFA.size(); i++)
	{
		out << "State " << i << endl;
		for (auto it = DFA[i].begin(); it != DFA[i].end(); it++)
		{
			out << symbolTable[(*it).lhs] << " ::= ";
			for (int j = 0; j < (*it).rhs.size(); j++)
			{
				if (j == (*it).pos)
					out << " dot ";
				out << symbolTable[(*it).rhs[j]] << ' ';
			}
			if ((*it).pos == (*it).rhs.size())
				out << " dot ";
			out << "," << symbolTable[(*it).forecast];
			out << endl;
		}
	}
}

//���LR������
void Parsing::outputAnalyseTable(ofstream& out) {
	// ��ӡ��ͷ
	out << "State";
	for (size_t i = 0; i < symbolTable.size(); ++i) {
		out << "\t" << symbolTable[i];
	}
	out << endl;

	// ����ÿ��״̬
	for (size_t i = 0; i < analyseTable.size(); ++i) {
		// ��ӡ��ǰ״̬���
		out << i;
		// ����ÿ������
		for (size_t j = 0; j < analyseTable[i].size(); ++j) {
			// ��ȡ��ǰ״̬�£���ǰ���ŵĶ���
			auto action = analyseTable[i][j];
			if (action.first != '\0') { // ����ж���
				if (action.first == 's') {
					out << "\ts" << action.second; // �ƽ�����
				}
				else if (action.first == 'r') {
					out << "\tr" << action.second; // ��Լ����
				}
				else if (action.first == 'a') {
					out << "\tacc"; // ���ܶ���
				}
				else {
					out << "\t"; // ��Ӧ�÷�������������λ
				}
			}
			else {
				out << "\t"; // �޶���
			}
		}
		out << endl;
	}
}

//���
void Parsing::clear()
{
	symbolTable.clear();
	symbol2Index.clear();
	syntaxTable.clear();
	searchSyntaxByLhs.clear();
	firstTable.clear();
	DFA.clear();
	analyseTable.clear();
	syntaxTree.clear();
	statusStack = stack<DFA_statusIndex>();
	analyseSymbolStack = stack<syntaxTreeNodeIndex>();
	inputSymbolvector = stack<syntaxTreeNodeIndex>();
}

//���������ķ����г�ʼ��
void Parsing::initSyntax(ifstream& fin)
{
	this->initSymbolTable(fin);//�������ű���ķ���
	this->initFirstTable();//��FIRST����
	this->initAnalyseTable();//����DFA��Ŀ��������LR������
}

//�﷨����
bool Parsing::analyze(const vector<TokenInfo>& lexs)
{
	//��ʼ���������ջ
	this->syntaxTree.push_back(TokenInfo{Token::End, "", 0, 0});
	this->syntaxTree.back().index = syntaxTree.size() - 1;//��¼����ֵ
	this->inputSymbolvector.push(syntaxTree.size() - 1);//����ֵ�������ջ
	for (int i = lexs.size() - 1; i >= 0; i--)//token�������ջ�У�����ͬ��
	{
		this->syntaxTree.push_back(lexs[i]);
		syntaxTreeNodeIndex index = syntaxTree.size() - 1;
		this->syntaxTree.back().index = index;
		this->inputSymbolvector.push(index);
	}
	//��ʼ������״̬ջ
	this->statusStack.push(0);
	//��ʼ����������ջ
	this->syntaxTree.push_back(TokenInfo{ Token::End, "", 0, 0 });
	this->syntaxTree.back().index = syntaxTree.size() - 1;
	this->analyseSymbolStack.push(syntaxTree.size() - 1);
	//�������̷��ű����
	this->p_symbolTable = new proc_symbolTable();
	//��ʼ����
	while (1)
	{
		//��������ջ��ջ������ʼ���ţ��������ջ��ջ���ǽ������ţ����﷨�������
		if (this->syntaxTree[this->analyseSymbolStack.top()].type == this->symbol2Index["$Program"] && this->syntaxTree[this->inputSymbolvector.top()].type == this->symbol2Index["$end"])break;
		//����״̬ջջ��״̬�Լ��������ջջ�����ʹӷ������еõ���һ������
		analyseTableItem nextAction = this->analyseTable[this->statusStack.top()][this->syntaxTree[this->inputSymbolvector.top()].type];
		int now_line = this->syntaxTree[this->inputSymbolvector.top()].line;
		int now_pos = this->syntaxTree[this->inputSymbolvector.top()].position;
		string now_val = this->syntaxTree[this->inputSymbolvector.top()].val;
		if (nextAction.first == 'a') break;//�﷨�����ɹ�
		else if (nextAction.first == 's')  //�ƽ�
		{
			this->statusStack.push(nextAction.second);
			syntaxTreeNodeIndex tmp = this->inputSymbolvector.top();
			this->inputSymbolvector.pop();
			this->analyseSymbolStack.push(tmp);
		}
		else if (nextAction.first == 'r') //��Լ
		{
			const syntaxTableItem& useSyntax = this->syntaxTable[nextAction.second];//��Լ��Ӧ����ʽ
			vector<syntaxTreeNodeIndex> rhsTmp;
			if (useSyntax.rhs[0] != this->emptyIndex)//��Լ����ʽ�Ҳ���Ϊ��
			{
				for (int i = 0; i < useSyntax.rhs.size(); i++)
				{
					syntaxTreeNodeIndex tmp = this->analyseSymbolStack.top();
					this->analyseSymbolStack.pop();
					this->statusStack.pop();
					rhsTmp.push_back(tmp);
					syntaxTree[tmp].parent = syntaxTree.size();//��¼��ջ�ַ����﷨���еĸ��ڵ�����
				}
			}
			syntaxTreeNode lhsTmp;
			reverse(rhsTmp.begin(), rhsTmp.end());//��ת��������
			lhsTmp.children = rhsTmp;
			lhsTmp.productions = nextAction.second;
			lhsTmp.type = useSyntax.lhs;
			lhsTmp.line = now_line;
			lhsTmp.val = Index2symbol[useSyntax.lhs];
			bool now_pd = this->generate_midcode(nextAction.second, lhsTmp, rhsTmp);//�����м����
			if (!now_pd) {
				cout << "�����������ȷ������������ȷ��Դ����" << endl;
				return false;
			}
			syntaxTree.push_back(lhsTmp);
			this->syntaxTree.back().index = syntaxTree.size() - 1;
			this->analyseSymbolStack.push(syntaxTree.size() - 1);//���󲿷������������ջ
			this->topNode = syntaxTree.size() - 1;
			//��Լ���������Ҫ����״̬��ջ
			nextAction = this->analyseTable[this->statusStack.top()][this->syntaxTree[this->analyseSymbolStack.top()].type];
			if (nextAction.first == 's') this->statusStack.push(nextAction.second);
			else
			{
				cout << "�﷨�������󣡴���λ�ڵ�" << this->syntaxTree[this->analyseSymbolStack.top()].line << "�У�����tokenΪ" 
					 << this->syntaxTree[this->analyseSymbolStack.top()].val << endl;
				cout << "�����������ȷ����������﷨�����Դ����" << endl;
				return false;
			}
		}
		else //����
		{
			cout << "�﷨�������󣡴���λ�ڵ�" << now_line << "�У�����tokenΪ" << now_val << endl;
			cout << "�����������ȷ����������﷨�����Դ����" << endl;
			return false;
		}
	}
	return true;
}

//�����м����
bool Parsing::generate_midcode(syntaxTableIndex SyntaxIndex, syntaxTreeNode& lhs, vector<syntaxTreeNodeIndex>& rhs)
{
	proc_symbolTable* functmp;
	switch (SyntaxIndex)
	{
		case 0: //$Program ::= <N> <������>
			mid_code.back_patch(syntaxTree[rhs[0]].nextlist, this->p_symbolTable->find_function("main")->get_enterquad());
			break;
		case 1: //<A> ::= $Empty
			lhs.quad = mid_code.nextquad;
			p_symbolTable = p_symbolTable->create_function(); //���뺯��
			break;
		case 2: //<N> ::= $Empty
			lhs.nextlist.push_back(mid_code.nextquad);
			mid_code.emit_code(quadruple(Oper::J, string(""), string(""), string("")));
			break;
		case 3: //<M> ::= $Empty
			lhs.quad = mid_code.nextquad;
			break;
		case 4: //<������> ::= <����>
			break;
		case 5: //<������> ::= <������> <����>
			break;
		case 6: //<����> ::= $Int $ID <��������>
			p_symbolTable->insert_variable({ symbolType::Int, syntaxTree[rhs[1]].val, proc_symbolTable::newtemp(), p_symbolTable->get_offset() });
			break;
		case 7: //<����> ::= $Void $ID <M> <A> <��������>
			beforeSymbolTable->init_function(syntaxTree[rhs[4]].plist, syntaxTree[rhs[1]].val, symbolType::Void, syntaxTree[rhs[2]].quad);
			break;
		case 8: //<����> ::= $Int $ID <M> <A> <��������>
			beforeSymbolTable->init_function(syntaxTree[rhs[4]].plist, syntaxTree[rhs[1]].val, symbolType::Int, syntaxTree[rhs[2]].quad);
			break;
		case 9: //<��������> ::=  $;
			break;
		case 10: //<��������> ::= $LeftBracket <�β�> $RightBracket <����>
			lhs.plist = syntaxTree[rhs[1]].plist;
			break;
		case 11: //<�β�> ::= <�����б�>
			lhs.plist = syntaxTree[rhs[0]].plist;
			break;
		case 12: //<�β�> ::= $Void
			lhs.plist = vector<symbolTableItem>();
			break;
		case 13: //<�����б�> ::= <����>
			lhs.plist.push_back(syntaxTree[rhs[0]].plist[0]);
			break;
		case 14: //<�����б�> ::= <����> $Comma <�����б�>
			lhs.plist.push_back(syntaxTree[rhs[0]].plist[0]);
			lhs.plist.insert(lhs.plist.end(), syntaxTree[rhs[2]].plist.begin(), syntaxTree[rhs[2]].plist.end());
			break;
		case 15: //<����> ::= $Int $ID
		{
			string parm_name = proc_symbolTable::newtemp();
			lhs.plist.push_back({ symbolType::Int, syntaxTree[rhs[1]].val, parm_name, p_symbolTable->get_offset() });
			p_symbolTable->insert_variable({ lhs.plist[0].type, lhs.plist[0].name, parm_name, p_symbolTable->get_offset() });
			break;
		}
		case 16: //<����> ::= $LeftBrace <�ڲ�����> <��䴮> $RightBrace
			lhs.nextlist = syntaxTree[rhs[2]].nextlist;
			beforeSymbolTable = p_symbolTable;
			p_symbolTable = p_symbolTable->return_block(mid_code.nextquad);
			break;
		case 17: //<�ڲ�����> ::= $Empty
			break;
		case 18: //<�ڲ�����> ::= <�ڲ���������> <�ڲ�����>
			break;
		case 19: //<�ڲ���������> ::= $Int $ID $;
			p_symbolTable->insert_variable({ symbolType::Int, syntaxTree[rhs[1]].val, proc_symbolTable::newtemp(), p_symbolTable->get_offset() });
			break;
		case 20: //<��䴮> ::= <���>
			lhs.nextlist = syntaxTree[rhs[0]].nextlist;
			break;
		case 21: //<��䴮> ::= <���> <M> <��䴮>
			lhs.nextlist = syntaxTree[rhs[2]].nextlist;
			mid_code.back_patch(syntaxTree[rhs[0]].nextlist, syntaxTree[rhs[1]].quad);
			break;
		case 22: //<���> ::= <if���>
			lhs.nextlist = syntaxTree[rhs[0]].nextlist;
			break;
		case 23: //<���> :: = <while���>
			lhs.nextlist = syntaxTree[rhs[0]].nextlist;
			break;
		case 24: //<���> ::= <return���>
			break;
		case 25: //<���> ::= <assign���>
			break;
		case 26: //<assign���> ::= $ID $Equal <���ʽ> $;
			if (p_symbolTable->find_variable(syntaxTree[rhs[0]].val).type == symbolType::None) {
				cout << "����������󣡴���λ�ڵ�" << syntaxTree[rhs[0]].line << "�У�" << syntaxTree[rhs[0]].val << "���ڷ��ű���" << endl;
				return false;
			}
			else
				mid_code.emit_code(quadruple(Oper::Assign, syntaxTree[rhs[2]].place, string(""), p_symbolTable->find_variable(syntaxTree[rhs[0]].val).gobalname));
			break;
		case 27: //<return���> ::= $Return $;
			mid_code.emit_code(quadruple(Oper::Return, string(""), string(""), string("")));
			break;
		case 28: //<return���> ::= $Return <���ʽ> $;
			mid_code.emit_code(quadruple(Oper::Return, syntaxTree[rhs[1]].place, string(""), string("ReturnValue")));
			break;
		case 29: //<while���> ::= $While <M> $LeftBracket <���ʽ> $RightBracket <A> <����>
			mid_code.back_patch(syntaxTree[rhs[6]].nextlist, syntaxTree[rhs[1]].quad);
			mid_code.back_patch(syntaxTree[rhs[3]].truelist, syntaxTree[rhs[5]].quad);
			lhs.nextlist = syntaxTree[rhs[3]].falselist;
			mid_code.emit_code(quadruple(Oper::J, string(""), string(""), to_string(syntaxTree[rhs[1]].quad)));
			break;
		case 30: //<if���> ::= $If $LeftBracket <���ʽ> $RightBracket <A> <����>
			mid_code.back_patch(syntaxTree[rhs[2]].truelist, syntaxTree[rhs[4]].quad);
			lhs.nextlist = mergelist(syntaxTree[rhs[2]].falselist, syntaxTree[rhs[4]].nextlist);
			break;
		case 31: //<if���> ::= $If $LeftBracket <���ʽ> $RightBracket <A> <����> <N> $Else <M> <A> <����>
			mid_code.back_patch(syntaxTree[rhs[2]].truelist, syntaxTree[rhs[4]].quad);
			mid_code.back_patch(syntaxTree[rhs[2]].falselist, syntaxTree[rhs[8]].quad);
			lhs.nextlist = mergelist(syntaxTree[rhs[5]].nextlist, syntaxTree[rhs[6]].nextlist, syntaxTree[rhs[10]].nextlist);
			break;
		case 32: //<���ʽ> ::= <�ӷ����ʽ>
			lhs.place = syntaxTree[rhs[0]].place;
			break;
		case 33: //<���ʽ> ::= <���ʽ> <�Ƚ������> <�ӷ����ʽ>
			lhs.truelist.push_back(mid_code.nextquad);
			lhs.falselist.push_back(mid_code.nextquad + 1);
			if (syntaxTree[rhs[1]].place == "$Smaller")
				mid_code.emit_code(quadruple(Oper::Jlt, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, string("0")));
			else if (syntaxTree[rhs[1]].place == "$SmallerEqual")
				mid_code.emit_code(quadruple(Oper::Jle, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, string("0")));
			else if (syntaxTree[rhs[1]].place == "$Bigger")
				mid_code.emit_code(quadruple(Oper::Jgt, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, string("0")));
			else if (syntaxTree[rhs[1]].place == "$BiggerEqual")
				mid_code.emit_code(quadruple(Oper::Jge, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, string("0")));
			else if (syntaxTree[rhs[1]].place == "$Equal2")
				mid_code.emit_code(quadruple(Oper::Jeq, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, string("0")));
			else if (syntaxTree[rhs[1]].place == "$NotEqual")
				mid_code.emit_code(quadruple(Oper::Jne, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, string("0")));
			else
				cout << "wrong comparation" << endl;
			mid_code.emit_code(quadruple(Oper::J, string(""), string(""), string("0")));
			break;
		case 34: //<�Ƚ������> ::= $Smaller
			lhs.place = "$Smaller";
			break;
		case 35: //<�Ƚ������> ::= $SmallerEqual
			lhs.place = "$SmallerEqual";
			break;
		case 36: //<�Ƚ������> ::= $Bigger
			lhs.place = "$Bigger";
			break;
		case 37: //<�Ƚ������> ::= $BiggerEqual
			lhs.place = "$BiggerEqual";
			break;
		case 38: //<�Ƚ������> ::= $Equal2
			lhs.place = "$Equal2";
			break;
		case 39: //<�Ƚ������> ::= $NotEqual
			lhs.place = "$NotEqual";
			break;
		case 40: //<�ӷ����ʽ> ::= <��>
			lhs.place = syntaxTree[rhs[0]].place;
			break;
		case 41: //<�ӷ����ʽ> ::= <��> $Plus <�ӷ����ʽ>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Plus, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 42: //<�ӷ����ʽ> ::= <��> $Minus <�ӷ����ʽ>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Minus, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 43: //<��> ::= <����>
			lhs.place = syntaxTree[rhs[0]].place;
			break;
		case 44: //<��> ::= <����> $Multiply <��>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Multiply, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 45: //<��> ::= <����> $Divide <��>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Divide, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 46: //<����> ::= $Number
			lhs.place = syntaxTree[rhs[0]].val;
			break;
		case 47: //<����> ::= $LeftBracket <���ʽ> $RightBracket
			lhs.place = syntaxTree[rhs[1]].place;
			break;
		case 48: //<����> ::= $ID $LeftBracket <ʵ���б�> $RightBracket
			functmp = p_symbolTable->find_function(syntaxTree[rhs[0]].val);//�����ڷ��ű��в��Ҹú�����
			if (functmp == NULL) {
				cout << "����������󣡴���λ�ڵ�" << syntaxTree[rhs[0]].line << "�У�" << syntaxTree[rhs[0]].val << "���ں�������" << endl;
				return false;
			}
			else if (functmp->parm.size() != syntaxTree[rhs[2]].plist.size()) {//Ȼ���жϲ��������Ƿ�ƥ��
				cout << "����������󣡴���λ�ڵ�" << syntaxTree[rhs[0]].line << "�У�" << syntaxTree[rhs[0]].val << "����ʵ���б���" << endl;
				return false;
			}
			else
			{
				for (const auto& i : syntaxTree[rhs[2]].plist)
				{
					mid_code.emit_code(quadruple(Oper::Parm, i.gobalname, string(""), syntaxTree[rhs[0]].val));
				}
				if (functmp->return_type == symbolType::Int)
				{
					lhs.place = p_symbolTable->newtemp();
					mid_code.emit_code(quadruple(Oper::Call, syntaxTree[rhs[0]].val, string(""), lhs.place));
				}
				else
				{
					mid_code.emit_code(quadruple(Oper::Call, syntaxTree[rhs[0]].val, string(""), string("")));
				}
			}
			break;
		case 49: //<����> ::= $ID
			if (p_symbolTable->find_variable(syntaxTree[rhs[0]].val).type == symbolType::None) {
				cout << "����������󣡴���λ�ڵ�" << syntaxTree[rhs[0]].line << "�У�" << syntaxTree[rhs[0]].val << "���ڷ��ű���" << endl;
				return false;
			}
			else
				lhs.place = p_symbolTable->find_variable(syntaxTree[rhs[0]].val).gobalname;
			break;
		case 50: //<ʵ���б�> ::= $Empty
			break;
		case 51: //<ʵ���б�> ::= <���ʽ>
			lhs.plist.push_back({ symbolType::Unknown, syntaxTree[rhs[0]].place, syntaxTree[rhs[0]].place, 0 });
			break;
		case 52: //<ʵ���б�> ::= <���ʽ> $Comma <ʵ���б�>
			lhs.plist.push_back({ symbolType::Unknown, syntaxTree[rhs[0]].place, syntaxTree[rhs[0]].place, 0 });
			lhs.plist.insert(lhs.plist.end(), syntaxTree[rhs[2]].plist.begin(), syntaxTree[rhs[2]].plist.end());
			break;
		default:
			break;
	}
	return true;
}

//����﷨���ṹ dot��ʽ
void Parsing::outputDot(ofstream& graph, syntaxTreeNodeIndex Node)
{
	if (!this->syntaxTree[Node].children.empty())//���ӽڵ�
	{
		//���ÿ���ӽڵ�
		for (auto child : this->syntaxTree[Node].children)
			graph << "	" << "Node" << Node << "->" << "Node" << child << endl;
		//����ж���ӽڵ㣬Ҫע��ͼ�θ�ʽ
		if (this->syntaxTree[Node].children.size() > 1)
		{
			graph << "	" << "{" << endl << "		" << "rank = same;" << endl << "		";
			for (int i = 0; i < this->syntaxTree[Node].children.size(); i++)
			{
				graph << "Node" << this->syntaxTree[Node].children[i];
				if (i < this->syntaxTree[Node].children.size() - 1)graph << "->";
			}
			graph << "[color=white];" << endl << "		" << "rankdir=LR;" << endl << "	" << "}" << endl;
		}
		//�ݹ鴦��ÿһ���ӽڵ�
		for (auto child : this->syntaxTree[Node].children)this->outputDot(graph, child);
	}
}

//����﷨��
void Parsing::outputSTree(ofstream& graph)
{
	//����﷨�� dot��ʽ
	graph << "#@startdot" << endl << endl;
	graph << "digraph demo {" << endl << "node [fontname=\"Fangsong\" shape=plaintext]" << endl << endl;
	for (int i = 0; i < syntaxTree.size(); i++)
		if (this->syntaxTree[i].inTree)
			graph << "	" << "Node" << i << "[label=\"" << this->symbolTable[this->syntaxTree[i].type] << "\", shape=\"box\"]" << endl;
	graph << endl;
	this->outputDot(graph, this->topNode);
	graph << endl << "}" << endl << endl;
	graph << "#@enddot" << endl;
	//���png��ʽ
	if (!system("dot -Tpng SyntaxTree.dot -o SyntaxTree.png"));
	else cout << "�﷨���ı�SyntaxTree.dot�����ɣ���ȱ�ٻ������ɶ�ӦͼƬ��" << endl;
}

//����м����
void Parsing::outputMidcode(ofstream& midcode)
{
	this->mid_code.output(midcode);
}

//�ϲ�������
vector<quadrupleIndex> Parsing::mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2)
{
	vector<quadrupleIndex> temp;
	temp.insert(temp.end(), list1.begin(), list1.end());
	temp.insert(temp.end(), list2.begin(), list2.end());
	return temp;
}

//�ϲ�������
vector<quadrupleIndex> Parsing::mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2, vector<quadrupleIndex>& list3)
{
	vector<quadrupleIndex> temp;
	temp.insert(temp.end(), list1.begin(), list1.end());
	temp.insert(temp.end(), list2.begin(), list2.end());
	temp.insert(temp.end(), list3.begin(), list3.end());
	return temp;
}

//���ط��ű�ָ��
proc_symbolTable* Parsing::get_proc_symbolTable()const
{
	return this->p_symbolTable;
}
