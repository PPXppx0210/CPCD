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

//插入，返回索引
symbolTableIndex Parsing::insertSymbol(symbolItem insrt)
{
	if (!this->symbol2Index.count(insrt)) // map中不存在
	{
		symbolTable.push_back(insrt);
		symbol2Index[insrt] = symbolTable.size() - 1;
		Index2symbol[symbolTable.size() - 1] = insrt;
	}
	return symbol2Index[insrt];
}

//初始化终结符集合
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
	this->terminalSymbolMax = symbolTable.size() - 1 - 2;//终结符最大索引
	this->startIndex = this->symbol2Index["$Program0"];//起始符号索引
	this->emptyIndex = this->symbol2Index["$empty"];//空符号索引
}

//初始化 symbolTable、terminalSymbolMax、startIndexsymbol2Index、syntaxTable
void Parsing::initSymbolTable(ifstream& infile)
{
	initTerminalSymbol();
	int line = 0;
	char templine[1024]; //存一行
	string tmpstr;
	while (infile.getline(templine, 1024))//逐行读取语法文件
	{
		line++;
		syntaxTableItem tmpSyntax;
		stringstream ss(templine);
		if (ss >> tmpstr)//每次读一个string类型
		{
			if (!((tmpstr[0] == '<' && tmpstr.back() == '>') || tmpstr[0] == '$'))
				cout << "语法规则错误: "<< "语法输入第" << line << "行左侧不是非终结符" << tmpstr << endl;
			tmpSyntax.lhs = insertSymbol(tmpstr);
			ss >> tmpstr;
			if (tmpstr != "::=")
				cout << "语法规则错误: "<< "语法输入第" << line << "行未找到赋值符(::=)，现为" << tmpstr << endl;
			while (ss >> tmpstr) tmpSyntax.rhs.push_back(insertSymbol(tmpstr));
			this->syntaxTable.push_back(tmpSyntax);
		}
	}
	this->syntaxTable.push_back({ this->startIndex, {this->symbol2Index["$Program"]} }); //添加S' ==> S
	searchSyntaxByLhs = vector<set<syntaxTableIndex>>(symbolTable.size());
	for (int i = 0; i < syntaxTable.size(); i++)searchSyntaxByLhs[syntaxTable[i].lhs].insert(i);
}

//求每个符号的 FIRST 集
void Parsing::initFirstTable()
{
	this->firstTable = vector<firstTableItem>(symbolTable.size());
	//终结符的FIRST集为其自身
	for (int i = 0; i <= this->terminalSymbolMax; i++)this->firstTable[i].insert(i);
	//非终结符FIRST集加入产生式的左侧
	for (int i = this->terminalSymbolMax + 1; i < this->symbolTable.size(); i++)
	{
		for (auto syntaxIndexTmp : this->searchSyntaxByLhs[i])//遍历每一个相关的产生式
		{
			//产生式右侧首项为终结符，则直接加入
			symbolTableIndex symbolTmp = this->syntaxTable[syntaxIndexTmp].rhs[0];
			if (symbolTmp < this->terminalSymbolMax)this->firstTable[i].insert(symbolTmp); 
		}
	}
	//非终结符FIRST集互推
	bool inc = false;
	do
	{
		inc = false;
		for (int i = this->terminalSymbolMax + 1; i < this->symbolTable.size(); i++) //对于每一个非终结符集
		{
			int cntTmp = this->firstTable[i].size();
			for (auto syntaxIndexTmp : this->searchSyntaxByLhs[i]) //遍历每一个相关的产生式
			{
				const vector<symbolTableIndex>& rhsTmp = this->syntaxTable[syntaxIndexTmp].rhs;
				for (int rhsIndex = 0; rhsIndex < rhsTmp.size(); rhsIndex++) //遍历该产生式右项
				{
					const firstTableItem& firstSymbolSet = this->firstTable[rhsTmp[rhsIndex]];
					if (rhsTmp[rhsIndex] <= this->terminalSymbolMax) //当前符号为终结符
					{
						this->firstTable[i].insert(rhsTmp[rhsIndex]);
						break;
					}
					bool haveEmpty = this->firstTable[i].count(this->emptyIndex);
					//将当前符号的 FIRST 集中的所有元素插入到目标 FIRST 集中
					this->firstTable[i].insert(firstSymbolSet.cbegin(), firstSymbolSet.cend());
					//本来不含空 且 当前符号的 FIRST 集中含空，则把上一步加入的空符号先去除
					if (!haveEmpty && firstSymbolSet.count(this->emptyIndex))
						this->firstTable[i].erase(this->emptyIndex);
					if (!firstSymbolSet.count(this->emptyIndex)) break;//不含空则无需讨论下一个字符
					if (rhsIndex == rhsTmp.size() - 1) //如果产生式右侧全为空，才能加入空符号
						this->firstTable[i].insert(this->emptyIndex);
				}
			}
			if (this->firstTable[i].size() > cntTmp)inc = true;//是否增长
		}
	} while (inc); //直到不再增长
}

 //计算句子的FIRST集
set<symbolTableIndex> Parsing::firstForPhrase(vector<symbolTableIndex> rhsTmp)
{
	set<symbolTableIndex> res;
	for (int rhsIndex = 0; rhsIndex < rhsTmp.size(); rhsIndex++) //遍历每一个字符
	{
		const firstTableItem& firstSymbolSet = this->firstTable[rhsTmp[rhsIndex]];
		if (rhsTmp[rhsIndex] <= this->terminalSymbolMax) //当前字符为终结符
		{
			res.insert(rhsTmp[rhsIndex]);
			break;
		}
		bool haveEmpty = res.count(this->emptyIndex);
		res.insert(firstSymbolSet.cbegin(), firstSymbolSet.cend());
		if (!haveEmpty && firstSymbolSet.count(this->emptyIndex))
			res.erase(this->emptyIndex);
		if (!firstSymbolSet.count(this->emptyIndex)) //不含空
			break;
		if (rhsIndex == rhsTmp.size() - 1) //如果每一个字符的FIRST集都含空，才加入空符号
			res.insert(this->emptyIndex);
	}
	return res;
}

//重载<，用于比较DFA_item
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

 //重载==，用于比较DFA_item
bool operator==(const DFA_item& A, const DFA_item& B)
{
	if (A.lhs == B.lhs && A.rhs == B.rhs && A.pos == B.pos && A.forecast == B.forecast)
		return true;
	else
		return false;
}

//求状态集合的闭包
pair<int, bool> Parsing::createClosure(DFA_status& sta)
{
	set<int> tempfirst;
	vector<symbolTableIndex> restsentence;
	stack<DFA_item> sd;
	DFA_item temptop, tempd;
	//先把所有项目入栈
	for (auto it = sta.begin(); it != sta.end(); it++)sd.push(*it);
	while (!sd.empty())
	{
		temptop = sd.top();
		sd.pop();
		if (temptop.pos < temptop.rhs.size() && temptop.rhs[temptop.pos] > terminalSymbolMax) //点后面是非终结符
		{
			restsentence.clear();
			for (int i = temptop.pos + 1; i < temptop.rhs.size(); i++) //提取需要找first集的语句
				restsentence.push_back(temptop.rhs[i]);
			restsentence.push_back(temptop.forecast);
			tempfirst = firstForPhrase(restsentence); //求字符串restsentence的FIRST集
			for (auto it = tempfirst.begin(); it != tempfirst.end(); it++)//遍历FIRST集合
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
	//检查是否是新状态
	for (int i = 0; i < DFA.size(); i++)
		if (DFA[i] == sta) return pair<int, bool>(i, false);
	//是新状态，则加入DFA状态集合
	DFA.push_back(sta);
	return pair<int, bool>(DFA.size() - 1, true);
}

//求DFA项目集，填LR分析表
void Parsing::initAnalyseTable()
{
	DFA_status temps; //DFA状态集合
	DFA_item tempd;
	pair<int, bool> gt;
	int statusno;
	stack<int> si;
	//先手动构造初始状态0
	//第一条S'->.S,#
	tempd.lhs = symbol2Index["$Program0"];
	tempd.rhs.push_back(symbol2Index["$Program"]);
	tempd.pos = 0;
	tempd.forecast = symbol2Index["$end"];
	temps.insert(tempd);
	createClosure(temps); //创建0号项目集
	si.push(0); //把状态0入栈
	//开始推导剩余状态
	set<int> transflag; //记录有哪些符号可以用来转移
	analyseTable.push_back(vector<analyseTableItem>(symbolTable.size(), pair<char, int>('\0', -1)));//初始化分析表
	while (!si.empty())
	{
		statusno = si.top();
		// DFA_status& DFA[statusno] = DFA[statusno];
		si.pop();
		transflag.clear();
		for (auto it = DFA[statusno].begin(); it != DFA[statusno].end(); it++) //找到这个集合中所有可以转移的字符
		{
			if ((*it).pos < (*it).rhs.size())
				transflag.insert((*it).rhs[(*it).pos]);
		}
		for (auto it = transflag.begin(); it != transflag.end(); it++) //对于每个可引发转移的字符，找移进状态
		{
			if (*it == emptyIndex) //特殊处理生成empty的内容
			{
				;
			}
			else //正常转移写转移表
			{
				temps.clear();
				for (auto it2 = DFA[statusno].begin(); it2 != DFA[statusno].end(); it2++) //对于每一条语句
				{
					if ((*it2).pos < (*it2).rhs.size() && (*it2).rhs[(*it2).pos] == *it)//可以转移
					{
						tempd.lhs = (*it2).lhs;
						tempd.rhs = (*it2).rhs;
						tempd.pos = (*it2).pos + 1;//只有位置向前一步
						tempd.forecast = (*it2).forecast;
						temps.insert(tempd);//新项目加入项目集
					}
				}
				gt = createClosure(temps);//闭包操作
				analyseTable[statusno][*it] = pair<char, int>('s', gt.first);//更新分析表，移进
				if (gt.second == true) //是新的状态/项目集
				{
					si.push(gt.first);
					analyseTable.push_back(vector<analyseTableItem>(symbolTable.size(), pair<char, int>('\0', -1)));
				}
			}
		}
		for (auto it = DFA[statusno].begin(); it != DFA[statusno].end(); it++) //找规约状态
		{
			if ((*it).pos >= (*it).rhs.size() || (*it).rhs[0] == emptyIndex)//可归约条件
			{
				int synno;
				//找到规约文法产生式
				for (auto it2 = searchSyntaxByLhs[(*it).lhs].begin(); it2 != searchSyntaxByLhs[(*it).lhs].end(); it2++)
				{
					if (syntaxTable[*it2].rhs == (*it).rhs)
					{
						synno = *it2;
						break;
					}
				}
				//填规约表
				if (analyseTable[statusno][(*it).forecast].first != '\0')cout << "wrong" << endl;//该表项已经填过
				if ((*it).lhs == symbol2Index["$Program0"] && (*it).rhs[0] == symbol2Index["$Program"] && (*it).forecast == symbol2Index["$end"])
					analyseTable[statusno][(*it).forecast] = pair<char, int>('a', -1);//接受状态
				else
					analyseTable[statusno][(*it).forecast] = pair<char, int>('r', synno);//一般规约状态
			}
		}
	}
}

//输出dfa项目集
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

//输出LR分析表
void Parsing::outputAnalyseTable(ofstream& out) {
	// 打印表头
	out << "State";
	for (size_t i = 0; i < symbolTable.size(); ++i) {
		out << "\t" << symbolTable[i];
	}
	out << endl;

	// 遍历每个状态
	for (size_t i = 0; i < analyseTable.size(); ++i) {
		// 打印当前状态编号
		out << i;
		// 遍历每个符号
		for (size_t j = 0; j < analyseTable[i].size(); ++j) {
			// 获取当前状态下，当前符号的动作
			auto action = analyseTable[i][j];
			if (action.first != '\0') { // 如果有动作
				if (action.first == 's') {
					out << "\ts" << action.second; // 移进动作
				}
				else if (action.first == 'r') {
					out << "\tr" << action.second; // 规约动作
				}
				else if (action.first == 'a') {
					out << "\tacc"; // 接受动作
				}
				else {
					out << "\t"; // 不应该发生，但保留空位
				}
			}
			else {
				out << "\t"; // 无动作
			}
		}
		out << endl;
	}
}

//清空
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

//根据输入文法进行初始化
void Parsing::initSyntax(ifstream& fin)
{
	this->initSymbolTable(fin);//构建符号表和文法表
	this->initFirstTable();//求FIRST集合
	this->initAnalyseTable();//构造DFA项目集，构造LR分析表
}

//语法分析
bool Parsing::analyze(const vector<TokenInfo>& lexs)
{
	//初始化输入符号栈
	this->syntaxTree.push_back(TokenInfo{Token::End, "", 0, 0});
	this->syntaxTree.back().index = syntaxTree.size() - 1;//记录索引值
	this->inputSymbolvector.push(syntaxTree.size() - 1);//索引值加入符号栈
	for (int i = lexs.size() - 1; i >= 0; i--)//token倒序加入栈中，步骤同上
	{
		this->syntaxTree.push_back(lexs[i]);
		syntaxTreeNodeIndex index = syntaxTree.size() - 1;
		this->syntaxTree.back().index = index;
		this->inputSymbolvector.push(index);
	}
	//初始化分析状态栈
	this->statusStack.push(0);
	//初始化分析符号栈
	this->syntaxTree.push_back(TokenInfo{ Token::End, "", 0, 0 });
	this->syntaxTree.back().index = syntaxTree.size() - 1;
	this->analyseSymbolStack.push(syntaxTree.size() - 1);
	//创建过程符号表对象
	this->p_symbolTable = new proc_symbolTable();
	//开始分析
	while (1)
	{
		//分析符号栈的栈顶是起始符号，输入符号栈的栈顶是结束符号，则语法分析完成
		if (this->syntaxTree[this->analyseSymbolStack.top()].type == this->symbol2Index["$Program"] && this->syntaxTree[this->inputSymbolvector.top()].type == this->symbol2Index["$end"])break;
		//根据状态栈栈顶状态以及输入符号栈栈顶类型从分析表中得到下一步动作
		analyseTableItem nextAction = this->analyseTable[this->statusStack.top()][this->syntaxTree[this->inputSymbolvector.top()].type];
		int now_line = this->syntaxTree[this->inputSymbolvector.top()].line;
		int now_pos = this->syntaxTree[this->inputSymbolvector.top()].position;
		string now_val = this->syntaxTree[this->inputSymbolvector.top()].val;
		if (nextAction.first == 'a') break;//语法分析成功
		else if (nextAction.first == 's')  //移进
		{
			this->statusStack.push(nextAction.second);
			syntaxTreeNodeIndex tmp = this->inputSymbolvector.top();
			this->inputSymbolvector.pop();
			this->analyseSymbolStack.push(tmp);
		}
		else if (nextAction.first == 'r') //归约
		{
			const syntaxTableItem& useSyntax = this->syntaxTable[nextAction.second];//规约对应产生式
			vector<syntaxTreeNodeIndex> rhsTmp;
			if (useSyntax.rhs[0] != this->emptyIndex)//规约产生式右部不为空
			{
				for (int i = 0; i < useSyntax.rhs.size(); i++)
				{
					syntaxTreeNodeIndex tmp = this->analyseSymbolStack.top();
					this->analyseSymbolStack.pop();
					this->statusStack.pop();
					rhsTmp.push_back(tmp);
					syntaxTree[tmp].parent = syntaxTree.size();//记录出栈字符在语法树中的父节点索引
				}
			}
			syntaxTreeNode lhsTmp;
			reverse(rhsTmp.begin(), rhsTmp.end());//翻转才是正序
			lhsTmp.children = rhsTmp;
			lhsTmp.productions = nextAction.second;
			lhsTmp.type = useSyntax.lhs;
			lhsTmp.line = now_line;
			lhsTmp.val = Index2symbol[useSyntax.lhs];
			bool now_pd = this->generate_midcode(nextAction.second, lhsTmp, rhsTmp);//生成中间代码
			if (!now_pd) {
				cout << "程序结束，请确保输入语义正确的源程序！" << endl;
				return false;
			}
			syntaxTree.push_back(lhsTmp);
			this->syntaxTree.back().index = syntaxTree.size() - 1;
			this->analyseSymbolStack.push(syntaxTree.size() - 1);//把左部符号入分析符号栈
			this->topNode = syntaxTree.size() - 1;
			//规约操作的最后要把新状态入栈
			nextAction = this->analyseTable[this->statusStack.top()][this->syntaxTree[this->analyseSymbolStack.top()].type];
			if (nextAction.first == 's') this->statusStack.push(nextAction.second);
			else
			{
				cout << "语法分析错误！错误位于第" << this->syntaxTree[this->analyseSymbolStack.top()].line << "行，错误token为" 
					 << this->syntaxTree[this->analyseSymbolStack.top()].val << endl;
				cout << "程序结束，请确保输入符合语法规则的源程序！" << endl;
				return false;
			}
		}
		else //错误
		{
			cout << "语法分析错误！错误位于第" << now_line << "行，错误token为" << now_val << endl;
			cout << "程序结束，请确保输入符合语法规则的源程序！" << endl;
			return false;
		}
	}
	return true;
}

//生成中间代码
bool Parsing::generate_midcode(syntaxTableIndex SyntaxIndex, syntaxTreeNode& lhs, vector<syntaxTreeNodeIndex>& rhs)
{
	proc_symbolTable* functmp;
	switch (SyntaxIndex)
	{
		case 0: //$Program ::= <N> <声明串>
			mid_code.back_patch(syntaxTree[rhs[0]].nextlist, this->p_symbolTable->find_function("main")->get_enterquad());
			break;
		case 1: //<A> ::= $Empty
			lhs.quad = mid_code.nextquad;
			p_symbolTable = p_symbolTable->create_function(); //进入函数
			break;
		case 2: //<N> ::= $Empty
			lhs.nextlist.push_back(mid_code.nextquad);
			mid_code.emit_code(quadruple(Oper::J, string(""), string(""), string("")));
			break;
		case 3: //<M> ::= $Empty
			lhs.quad = mid_code.nextquad;
			break;
		case 4: //<声明串> ::= <声明>
			break;
		case 5: //<声明串> ::= <声明串> <声明>
			break;
		case 6: //<声明> ::= $Int $ID <声明类型>
			p_symbolTable->insert_variable({ symbolType::Int, syntaxTree[rhs[1]].val, proc_symbolTable::newtemp(), p_symbolTable->get_offset() });
			break;
		case 7: //<声明> ::= $Void $ID <M> <A> <函数声明>
			beforeSymbolTable->init_function(syntaxTree[rhs[4]].plist, syntaxTree[rhs[1]].val, symbolType::Void, syntaxTree[rhs[2]].quad);
			break;
		case 8: //<声明> ::= $Int $ID <M> <A> <函数声明>
			beforeSymbolTable->init_function(syntaxTree[rhs[4]].plist, syntaxTree[rhs[1]].val, symbolType::Int, syntaxTree[rhs[2]].quad);
			break;
		case 9: //<声明类型> ::=  $;
			break;
		case 10: //<函数声明> ::= $LeftBracket <形参> $RightBracket <语句块>
			lhs.plist = syntaxTree[rhs[1]].plist;
			break;
		case 11: //<形参> ::= <参数列表>
			lhs.plist = syntaxTree[rhs[0]].plist;
			break;
		case 12: //<形参> ::= $Void
			lhs.plist = vector<symbolTableItem>();
			break;
		case 13: //<参数列表> ::= <参数>
			lhs.plist.push_back(syntaxTree[rhs[0]].plist[0]);
			break;
		case 14: //<参数列表> ::= <参数> $Comma <参数列表>
			lhs.plist.push_back(syntaxTree[rhs[0]].plist[0]);
			lhs.plist.insert(lhs.plist.end(), syntaxTree[rhs[2]].plist.begin(), syntaxTree[rhs[2]].plist.end());
			break;
		case 15: //<参数> ::= $Int $ID
		{
			string parm_name = proc_symbolTable::newtemp();
			lhs.plist.push_back({ symbolType::Int, syntaxTree[rhs[1]].val, parm_name, p_symbolTable->get_offset() });
			p_symbolTable->insert_variable({ lhs.plist[0].type, lhs.plist[0].name, parm_name, p_symbolTable->get_offset() });
			break;
		}
		case 16: //<语句块> ::= $LeftBrace <内部声明> <语句串> $RightBrace
			lhs.nextlist = syntaxTree[rhs[2]].nextlist;
			beforeSymbolTable = p_symbolTable;
			p_symbolTable = p_symbolTable->return_block(mid_code.nextquad);
			break;
		case 17: //<内部声明> ::= $Empty
			break;
		case 18: //<内部声明> ::= <内部变量声明> <内部声明>
			break;
		case 19: //<内部变量声明> ::= $Int $ID $;
			p_symbolTable->insert_variable({ symbolType::Int, syntaxTree[rhs[1]].val, proc_symbolTable::newtemp(), p_symbolTable->get_offset() });
			break;
		case 20: //<语句串> ::= <语句>
			lhs.nextlist = syntaxTree[rhs[0]].nextlist;
			break;
		case 21: //<语句串> ::= <语句> <M> <语句串>
			lhs.nextlist = syntaxTree[rhs[2]].nextlist;
			mid_code.back_patch(syntaxTree[rhs[0]].nextlist, syntaxTree[rhs[1]].quad);
			break;
		case 22: //<语句> ::= <if语句>
			lhs.nextlist = syntaxTree[rhs[0]].nextlist;
			break;
		case 23: //<语句> :: = <while语句>
			lhs.nextlist = syntaxTree[rhs[0]].nextlist;
			break;
		case 24: //<语句> ::= <return语句>
			break;
		case 25: //<语句> ::= <assign语句>
			break;
		case 26: //<assign语句> ::= $ID $Equal <表达式> $;
			if (p_symbolTable->find_variable(syntaxTree[rhs[0]].val).type == symbolType::None) {
				cout << "语义分析错误！错误位于第" << syntaxTree[rhs[0]].line << "行，" << syntaxTree[rhs[0]].val << "不在符号表中" << endl;
				return false;
			}
			else
				mid_code.emit_code(quadruple(Oper::Assign, syntaxTree[rhs[2]].place, string(""), p_symbolTable->find_variable(syntaxTree[rhs[0]].val).gobalname));
			break;
		case 27: //<return语句> ::= $Return $;
			mid_code.emit_code(quadruple(Oper::Return, string(""), string(""), string("")));
			break;
		case 28: //<return语句> ::= $Return <表达式> $;
			mid_code.emit_code(quadruple(Oper::Return, syntaxTree[rhs[1]].place, string(""), string("ReturnValue")));
			break;
		case 29: //<while语句> ::= $While <M> $LeftBracket <表达式> $RightBracket <A> <语句块>
			mid_code.back_patch(syntaxTree[rhs[6]].nextlist, syntaxTree[rhs[1]].quad);
			mid_code.back_patch(syntaxTree[rhs[3]].truelist, syntaxTree[rhs[5]].quad);
			lhs.nextlist = syntaxTree[rhs[3]].falselist;
			mid_code.emit_code(quadruple(Oper::J, string(""), string(""), to_string(syntaxTree[rhs[1]].quad)));
			break;
		case 30: //<if语句> ::= $If $LeftBracket <表达式> $RightBracket <A> <语句块>
			mid_code.back_patch(syntaxTree[rhs[2]].truelist, syntaxTree[rhs[4]].quad);
			lhs.nextlist = mergelist(syntaxTree[rhs[2]].falselist, syntaxTree[rhs[4]].nextlist);
			break;
		case 31: //<if语句> ::= $If $LeftBracket <表达式> $RightBracket <A> <语句块> <N> $Else <M> <A> <语句块>
			mid_code.back_patch(syntaxTree[rhs[2]].truelist, syntaxTree[rhs[4]].quad);
			mid_code.back_patch(syntaxTree[rhs[2]].falselist, syntaxTree[rhs[8]].quad);
			lhs.nextlist = mergelist(syntaxTree[rhs[5]].nextlist, syntaxTree[rhs[6]].nextlist, syntaxTree[rhs[10]].nextlist);
			break;
		case 32: //<表达式> ::= <加法表达式>
			lhs.place = syntaxTree[rhs[0]].place;
			break;
		case 33: //<表达式> ::= <表达式> <比较运算符> <加法表达式>
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
		case 34: //<比较运算符> ::= $Smaller
			lhs.place = "$Smaller";
			break;
		case 35: //<比较运算符> ::= $SmallerEqual
			lhs.place = "$SmallerEqual";
			break;
		case 36: //<比较运算符> ::= $Bigger
			lhs.place = "$Bigger";
			break;
		case 37: //<比较运算符> ::= $BiggerEqual
			lhs.place = "$BiggerEqual";
			break;
		case 38: //<比较运算符> ::= $Equal2
			lhs.place = "$Equal2";
			break;
		case 39: //<比较运算符> ::= $NotEqual
			lhs.place = "$NotEqual";
			break;
		case 40: //<加法表达式> ::= <项>
			lhs.place = syntaxTree[rhs[0]].place;
			break;
		case 41: //<加法表达式> ::= <项> $Plus <加法表达式>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Plus, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 42: //<加法表达式> ::= <项> $Minus <加法表达式>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Minus, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 43: //<项> ::= <因子>
			lhs.place = syntaxTree[rhs[0]].place;
			break;
		case 44: //<项> ::= <因子> $Multiply <项>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Multiply, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 45: //<项> ::= <因子> $Divide <项>
			lhs.place = proc_symbolTable::newtemp();
			mid_code.emit_code(quadruple(Oper::Divide, syntaxTree[rhs[0]].place, syntaxTree[rhs[2]].place, lhs.place));
			break;
		case 46: //<因子> ::= $Number
			lhs.place = syntaxTree[rhs[0]].val;
			break;
		case 47: //<因子> ::= $LeftBracket <表达式> $RightBracket
			lhs.place = syntaxTree[rhs[1]].place;
			break;
		case 48: //<因子> ::= $ID $LeftBracket <实参列表> $RightBracket
			functmp = p_symbolTable->find_function(syntaxTree[rhs[0]].val);//首先在符号表中查找该函数名
			if (functmp == NULL) {
				cout << "语义分析错误！错误位于第" << syntaxTree[rhs[0]].line << "行，" << syntaxTree[rhs[0]].val << "不在函数表中" << endl;
				return false;
			}
			else if (functmp->parm.size() != syntaxTree[rhs[2]].plist.size()) {//然后判断参数个数是否匹配
				cout << "语义分析错误！错误位于第" << syntaxTree[rhs[0]].line << "行，" << syntaxTree[rhs[0]].val << "函数实参列表不符" << endl;
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
		case 49: //<因子> ::= $ID
			if (p_symbolTable->find_variable(syntaxTree[rhs[0]].val).type == symbolType::None) {
				cout << "语义分析错误！错误位于第" << syntaxTree[rhs[0]].line << "行，" << syntaxTree[rhs[0]].val << "不在符号表中" << endl;
				return false;
			}
			else
				lhs.place = p_symbolTable->find_variable(syntaxTree[rhs[0]].val).gobalname;
			break;
		case 50: //<实参列表> ::= $Empty
			break;
		case 51: //<实参列表> ::= <表达式>
			lhs.plist.push_back({ symbolType::Unknown, syntaxTree[rhs[0]].place, syntaxTree[rhs[0]].place, 0 });
			break;
		case 52: //<实参列表> ::= <表达式> $Comma <实参列表>
			lhs.plist.push_back({ symbolType::Unknown, syntaxTree[rhs[0]].place, syntaxTree[rhs[0]].place, 0 });
			lhs.plist.insert(lhs.plist.end(), syntaxTree[rhs[2]].plist.begin(), syntaxTree[rhs[2]].plist.end());
			break;
		default:
			break;
	}
	return true;
}

//输出语法树结构 dot形式
void Parsing::outputDot(ofstream& graph, syntaxTreeNodeIndex Node)
{
	if (!this->syntaxTree[Node].children.empty())//有子节点
	{
		//输出每个子节点
		for (auto child : this->syntaxTree[Node].children)
			graph << "	" << "Node" << Node << "->" << "Node" << child << endl;
		//如果有多个子节点，要注意图形格式
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
		//递归处理每一个子节点
		for (auto child : this->syntaxTree[Node].children)this->outputDot(graph, child);
	}
}

//输出语法树
void Parsing::outputSTree(ofstream& graph)
{
	//输出语法树 dot形式
	graph << "#@startdot" << endl << endl;
	graph << "digraph demo {" << endl << "node [fontname=\"Fangsong\" shape=plaintext]" << endl << endl;
	for (int i = 0; i < syntaxTree.size(); i++)
		if (this->syntaxTree[i].inTree)
			graph << "	" << "Node" << i << "[label=\"" << this->symbolTable[this->syntaxTree[i].type] << "\", shape=\"box\"]" << endl;
	graph << endl;
	this->outputDot(graph, this->topNode);
	graph << endl << "}" << endl << endl;
	graph << "#@enddot" << endl;
	//输出png形式
	if (!system("dot -Tpng SyntaxTree.dot -o SyntaxTree.png"));
	else cout << "语法树文本SyntaxTree.dot已生成，但缺少环境生成对应图片。" << endl;
}

//输出中间代码
void Parsing::outputMidcode(ofstream& midcode)
{
	this->mid_code.output(midcode);
}

//合并两条链
vector<quadrupleIndex> Parsing::mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2)
{
	vector<quadrupleIndex> temp;
	temp.insert(temp.end(), list1.begin(), list1.end());
	temp.insert(temp.end(), list2.begin(), list2.end());
	return temp;
}

//合并三条链
vector<quadrupleIndex> Parsing::mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2, vector<quadrupleIndex>& list3)
{
	vector<quadrupleIndex> temp;
	temp.insert(temp.end(), list1.begin(), list1.end());
	temp.insert(temp.end(), list2.begin(), list2.end());
	temp.insert(temp.end(), list3.begin(), list3.end());
	return temp;
}

//返回符号表指针
proc_symbolTable* Parsing::get_proc_symbolTable()const
{
	return this->p_symbolTable;
}
