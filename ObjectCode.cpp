#include "ObjectCode.h"
#include<iostream>
#include<queue>

NewLabeler::NewLabeler() :index(1), tmp(0)
{}

string NewLabeler::newLabel() {
	return string("Label") + to_string(index++);
}

string NewLabeler::newTmp() {
	return string("Tmp") + to_string(tmp++);
}

static bool isVar(string name) {
	return isalpha(name[0]);
}

static bool isNum(string name) {
	return isdigit(name[0]);
}

static bool isControlOp(string op) {
	if (op[0] == 'J' || op == "Call" || op == "Return" || op == "Get") {
		return true;
	}
	return false;
}

void ObjectCode::divideBlocks(const Parsing& pars, const proc_symbolTable* ptr) {
	vector<pair<int, string> > funcEnter = pars.p_symbolTable->getFuncEnter();
	this->code = pars.mid_code;

	for (vector<pair<int, string> >::iterator iter = funcEnter.begin(); iter != funcEnter.end(); iter++) { //每一个函数块
		vector<Block>blocks;
		priority_queue<int, vector<int>, greater<int> >block_enter; //所有基本块的入口位置
		block_enter.push(iter->first);

		int endIndex = (iter + 1 == funcEnter.end()) ? code.nextquad : (iter + 1)->first;
		for (int i = iter->first; i != endIndex; i++) {
			if (code.code[i - CODE_START_POS].Op == Oper::J || code.code[i - CODE_START_POS].Op == Oper::Jeq || code.code[i - CODE_START_POS].Op == Oper::Jge ||
				code.code[i - CODE_START_POS].Op == Oper::Jgt || code.code[i - CODE_START_POS].Op == Oper::Jle
				|| code.code[i - CODE_START_POS].Op == Oper::Jlt || code.code[i - CODE_START_POS].Op == Oper::Jne)
			{
				if (code.code[i - CODE_START_POS].Op == Oper::J)
				{
					block_enter.push(atoi(code.code[i - CODE_START_POS].result.c_str()));
				}
				else
				{
					if (i + 1 < endIndex) {
						block_enter.push(i + 1);
					}
					block_enter.push(atoi(code.code[i - CODE_START_POS].result.c_str()));
				}
			}
			else if (code.code[i - CODE_START_POS].Op == Oper::Return) {
				if (i + 1 < endIndex) {
					block_enter.push(i + 1);
				}
			}
		}

		//划分基本块	
		Block block;
		map<int, string>labelEnter; //入口点和标签的对应关系
		map<int, int>enter_block; //建立入口点和block的对应关系
		int firstFlag = true; //函数块第一块标记，该块命名为函数名
		int enter;
		int lastEnter = block_enter.top();
		block_enter.pop();
		while (!block_enter.empty()) {
			enter = block_enter.top();
			block_enter.pop();

			if (enter == lastEnter) {
				continue;
			}

			for (int i = lastEnter; i != enter; i++) {
				block.codes.push_back(code.code[i - CODE_START_POS]);
			}

			if (!firstFlag) { //该基本块不是函数块的第一块基本块
				block.name = label.newLabel();
				labelEnter[lastEnter] = block.name;
			}
			else { //该基本块是函数块的第一块基本块
				block.name = iter->second;
				firstFlag = false;
			}

			enter_block[lastEnter] = blocks.size();
			blocks.push_back(block);
			lastEnter = enter;
			block.codes.clear();
		}
		if (!firstFlag) { //该基本块不是函数块的第一块基本块
			block.name = label.newLabel();
			labelEnter[lastEnter] = block.name;
		}
		else { //该基本块是函数块的第一块基本块
			block.name = iter->second;
			firstFlag = false;
		}
		if (iter + 1 != funcEnter.end()) { //在两个函数的起点之间
			for (int i = lastEnter; i != (iter + 1)->first; i++) {
				block.codes.push_back(code.code[i - CODE_START_POS]);
			}
		}
		else {//在最后一个函数至中间代码末尾
			for (int i = lastEnter; i != code.nextquad; i++) {
				block.codes.push_back(code.code[i - CODE_START_POS]);
			}
		}
		enter_block[lastEnter] = blocks.size();
		blocks.push_back(block);

		int blockIndex = 0;
		for (vector<Block>::iterator bIter = blocks.begin(); bIter != blocks.end(); bIter++, blockIndex++) {
			vector<quadruple>::reverse_iterator lastCode = bIter->codes.rbegin();
			if (lastCode->Op == Oper::J || lastCode->Op == Oper::Jeq || lastCode->Op == Oper::Jge || lastCode->Op == Oper::Jgt
				|| lastCode->Op == Oper::Jle || lastCode->Op == Oper::Jlt || lastCode->Op == Oper::Jne)
			{
				if (lastCode->Op == Oper::J) {
					bIter->next1 = enter_block[atoi(lastCode->result.c_str())];
					bIter->next2 = -1;
				}
				else {
					bIter->next1 = blockIndex + 1;
					bIter->next2 = enter_block[atoi(lastCode->result.c_str())];
					bIter->next2 = bIter->next1 == bIter->next2 ? -1 : bIter->next2;
				}
				lastCode->result = labelEnter[atoi(lastCode->result.c_str())];
			}
			else if (lastCode->Op == Oper::Return) {
				bIter->next1 = bIter->next2 = -1;
			}
			else {
				bIter->next1 = blockIndex + 1;
				bIter->next2 = -1;
			}
		}
		funcBlocks[iter->second] = blocks;
	}

	for (map<string, vector<Block> >::iterator fbiter = funcBlocks.begin(); fbiter != funcBlocks.end(); fbiter++)
	{
		const std::vector<symbolTableItem>& parms = ptr->functionTable.at(fbiter->first)->parm;
		Block& block = fbiter->second[0];
		vector<quadruple> get_parm;

		if (fbiter->first != string("main"))
		{
			for (const auto& parm : parms)
			{
				if (parm.type != symbolType::Array)
					get_parm.push_back({ Oper::Get,string(""),string("") ,parm.gobalname });
			}
		}
		block.codes.insert(block.codes.begin(), get_parm.begin(), get_parm.end());
	}
}

void ObjectCode::outputBlocks(ostream& out)
{
	for (map<string, vector<Block> >::iterator iter = funcBlocks.begin(); iter != funcBlocks.end(); iter++) {
		out << "[" << iter->first << "]" << endl;
		for (vector<Block>::iterator bIter = iter->second.begin(); bIter != iter->second.end(); bIter++) {
			out << bIter->name << ":" << endl;
			for (vector<quadruple>::iterator cIter = bIter->codes.begin(); cIter != bIter->codes.end(); cIter++) {
				out << "    " << "(" << Oper2string(cIter->Op) << "," << cIter->arg1 << "," << cIter->arg2 << "," << cIter->result << ")" << endl;
			}
			out << "    " << "next1 = " << bIter->next1 << endl;
			out << "    " << "next2 = " << bIter->next2 << endl;
		}
		out << endl;
	}
}

void ObjectCode::init_INOUTL()
{
	for (map<string, vector<Block> >::iterator fbiter = this->funcBlocks.begin(); fbiter != this->funcBlocks.end(); fbiter++) {
		vector<Block>& blocks = fbiter->second;
		vector<set<string> >INL, OUTL, DEF, USE;

		//活跃变量的数据流方程
		for (vector<Block>::iterator biter = blocks.begin(); biter != blocks.end(); biter++) {
			set<string>def, use;
			for (vector<quadruple>::iterator citer = biter->codes.begin(); citer != biter->codes.end(); citer++) {
				if (citer->Op == Oper::ArrayAssign) {
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//如果源操作数1还没有被定值
						use.insert(citer->arg1);
					}
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//如果源操作数2还没有被定值
						use.insert(citer->arg2);
					}
					if (isVar(citer->result) && use.count(citer->result) == 0) {//如果目的操作数还没有被引用
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::Get) {
					if (isVar(citer->result) && use.count(citer->result) == 0) {//如果目的操作数还没有被引用
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::AssignArray) {
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//如果源操作数2还没有被定值
						use.insert(citer->arg2);
					}
					if (isVar(citer->result) && use.count(citer->result) == 0) {//如果目的操作数还没有被引用
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::Call) {
					if (isVar(citer->result) && use.count(citer->result) == 0) {//如果目的操作数还没有被引用
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::Parm || citer->Op == Oper::Return) {
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//如果源操作数1还没有被定值
						use.insert(citer->arg1);
					}
				}
				else if (citer->Op == Oper::J)
				{
					//pass
				}
				else if (citer->Op == Oper::Jeq || citer->Op == Oper::Jge || citer->Op == Oper::Jgt || citer->Op == Oper::Jle
					|| citer->Op == Oper::Jlt || citer->Op == Oper::Jne)
				{
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//如果源操作数1还没有被定值
						use.insert(citer->arg1);
					}
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//如果源操作数2还没有被定值
						use.insert(citer->arg2);
					}
				}
				else {
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//如果源操作数1还没有被定值
						use.insert(citer->arg1);
					}
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//如果源操作数2还没有被定值
						use.insert(citer->arg2);
					}
					if (isVar(citer->result) && use.count(citer->result) == 0) {//如果目的操作数还没有被引用
						def.insert(citer->result);
					}
				}
			}
			INL.push_back(use);
			DEF.push_back(def);
			USE.push_back(use);
			if (biter->codes.back().Op == Oper::Return)
				OUTL.push_back(set<string>({ biter->codes.back().arg1 }));
			else
				OUTL.push_back(set<string>());
		}

		//确定INL，OUTL
		bool change = true;
		while (change) {
			change = false;
			int blockIndex = 0;
			for (vector<Block>::iterator biter = blocks.begin(); biter != blocks.end(); biter++, blockIndex++) {
				int next1 = biter->next1;
				int next2 = biter->next2;
				if (next1 != -1) {
					for (set<string>::iterator inlIter = INL[next1].begin(); inlIter != INL[next1].end(); inlIter++) {
						if (OUTL[blockIndex].insert(*inlIter).second == true) {
							if (DEF[blockIndex].count(*inlIter) == 0) {
								INL[blockIndex].insert(*inlIter);
							}
							change = true;
						}
					}
				}
				if (next2 != -1) {
					for (set<string>::iterator inlIter = INL[next2].begin(); inlIter != INL[next2].end(); inlIter++) {
						if (OUTL[blockIndex].insert(*inlIter).second == true) {
							if (DEF[blockIndex].count(*inlIter) == 0) {
								INL[blockIndex].insert(*inlIter);
							}
							change = true;
						}
					}
				}
			}
		}
		funcOUTL[fbiter->first] = OUTL;
		funcINL[fbiter->first] = INL;
	}
}

VarInfomation::VarInfomation(int next, bool active) {
	this->next = next;
	this->active = active;
}

VarInfomation::VarInfomation(const VarInfomation& other) {
	this->active = other.active;
	this->next = other.next;
}

VarInfomation::VarInfomation() :next(0), active(false)
{

}

void VarInfomation::output(ostream& out) {
	out << "(";
	if (next == -1)
		out << "^";
	else
		out << next;
	out << ",";
	if (active)
		out << "y";
	else
		out << "^";

	out << ")";
}

QuaternaryWithInfo::QuaternaryWithInfo(quadruple q, VarInfomation info1, VarInfomation info2, VarInfomation info3) {
	this->q = q;
	this->info1 = info1;
	this->info2 = info2;
	this->info3 = info3;
}

void QuaternaryWithInfo::output(ostream& out) {
	out << "(" << Oper2string(q.Op) << "," << q.arg1 << "," << q.arg2 << "," << q.result << ")";
	info1.output(out);
	info2.output(out);
	info3.output(out);
}

ObjectCode::ObjectCode() :top(0)
{
}

//将reg中的var变量存入内存
void ObjectCode::storeVar(string reg, string var) {
	if (reg == "")return;
	if (varOffset.find(var) != varOffset.end()) {//如果已经为*iter分配好了存储空间
		objectCodes.push_back(string("sw ") + reg + " " + to_string(varOffset[var]) + "($sp)");
	}
	else {
		varOffset[var] = top;
		top += 4;
		objectCodes.push_back(string("sw ") + reg + " " + to_string(varOffset[var]) + "($sp)");
	}
	Avalue[var].insert(var);
}

//释放寄存器
void ObjectCode::releaseVar(string var) {
	for (set<string>::iterator iter = Avalue[var].begin(); iter != Avalue[var].end(); iter++) {
		if ((*iter)[0] == '$') {
			Rvalue[*iter].erase(var);
			if (Rvalue[*iter].size() == 0 && (*iter)[1] == 's') {
				freeReg.push_back(*iter);
			}
		}
	}
	Avalue[var].clear();
}

//为引用变量分配寄存器
string ObjectCode::allocateReg() {
	//如果有尚未分配的寄存器，则从中选取一个
	string ret;
	if (freeReg.size()) {
		ret = freeReg.back();
		freeReg.pop_back();
		return ret;
	}
	/*
	从已分配的寄存器中选取一个Ri为所需要的寄存器R。最好使得Ri满足以下条件：
	占用Ri的变量的值也同时存放在该变量的贮存单元中
	或者在基本块中要在最远的将来才会引用到或不会引用到。
	*/
	const int inf = 1000000;
	int maxNextPos = 0;
	for (map<string, set<string> >::iterator iter = Rvalue.begin(); iter != Rvalue.end(); iter++) {//遍历所有的寄存器
		int nextpos = inf;
		for (set<string>::iterator viter = iter->second.begin(); viter != iter->second.end(); viter++) {//遍历寄存器中储存的变量
			bool inFlag = false;//变量已在其他地方存储的标志
			for (set<string>::iterator aiter = Avalue[*viter].begin(); aiter != Avalue[*viter].end(); aiter++) {//遍历变量的存储位置
				if (*aiter != iter->first) {//如果变量存储在其他地方
					inFlag = true;
					break;
				}
			}
			if (!inFlag) {//如果变量仅存储在寄存器中，就看未来在何处会引用该变量
				for (vector<QuaternaryWithInfo>::iterator cIter = nowQuatenary; cIter != nowIBlock->codes.end(); cIter++) {
					if (*viter == cIter->q.arg1 || *viter == cIter->q.arg2) {
						nextpos = cIter - nowQuatenary;
					}
					else if (*viter == cIter->q.result) {
						break;
					}
				}
			}
		}
		if (nextpos == inf) {
			ret = iter->first;
			break;
		}
		else if (nextpos > maxNextPos) {
			maxNextPos = nextpos;
			ret = iter->first;
		}
	}

	for (set<string>::iterator iter = Rvalue[ret].begin(); iter != Rvalue[ret].end(); iter++) {
		//对ret的寄存器中保存的变量*iter，他们都将不再存储在ret中
		Avalue[*iter].erase(ret);
		//如果V的地址描述数组AVALUE[V]说V还保存在R之外的其他地方，则不需要生成存数指令
		if (Avalue[*iter].size() > 0) {
			//pass
		}
		//如果V不会在此之后被使用，则不需要生成存数指令
		else {
			bool storeFlag = true;
			vector<QuaternaryWithInfo>::iterator cIter;
			for (cIter = nowQuatenary; cIter != nowIBlock->codes.end(); cIter++) {
				if (cIter->q.arg1 == *iter || cIter->q.arg2 == *iter) {//如果V在本基本块中被引用
					storeFlag = true;
					break;
				}
				if (cIter->q.result == *iter) {//如果V在本基本块中被赋值
					storeFlag = false;
					break;
				}
			}
			if (cIter == nowIBlock->codes.end()) {//如果V在本基本块中未被引用，且也没有被赋值
				int index = nowIBlock - funcIBlocks[nowFunc].begin();
				if (funcOUTL[nowFunc][index].count(*iter) == 1) {//如果此变量是出口之后的活跃变量
					storeFlag = true;
				}
				else {
					storeFlag = false;
				}
			}
			if (storeFlag) {//生成存数指令
				storeVar(ret, *iter);
			}
		}
	}
	Rvalue[ret].clear();//清空ret寄存器中保存的变量

	return ret;
}

//为引用变量分配寄存器
string ObjectCode::allocateReg(string var) {
	//为常数分配寄存器
	if (isNum(var)) {
		string ret = allocateReg();//任意空闲寄存器
		objectCodes.push_back(string("addi ") + ret + " $zero " + var);
		return ret;
	}
	for (set<string>::iterator iter = Avalue[var].begin(); iter != Avalue[var].end(); iter++) {
		if ((*iter)[0] == '$') {//如果变量已经保存在某个寄存器中
			//if (var == "T18")cout << "&&&" << endl << *iter << endl;
			return *iter;//直接返回该寄存器
		}
	}
	//如果该变量没有在某个寄存器中
	string ret = allocateReg();
	objectCodes.push_back(string("lw ") + ret + " " + to_string(varOffset[var]) + "($sp)");
	Avalue[var].insert(ret);
	Rvalue[ret].insert(var);
	return ret;
}

//为运算结果分配寄存器
string ObjectCode::getReg() {
	//i: A:=B op C
	//如果B的现行值在某个寄存器Ri中，RVALUE[Ri]中只包含B
	//此外，或者B与A是同一个标识符或者B的现行值在执行四元式A:=B op C之后不会再引用
	//则选取Ri为所需要的寄存器R

	//如果src1不是数字
	if (!isNum(nowQuatenary->q.arg1)) {
		//遍历src1所在的寄存器
		set<string>& src1pos = Avalue[nowQuatenary->q.arg1];
		for (set<string>::iterator iter = src1pos.begin(); iter != src1pos.end(); iter++) {
			if ((*iter)[0] == '$') {
				if (Rvalue[*iter].size() == 1) {//如果该寄存器中值仅仅存有src1
					if (nowQuatenary->q.result == nowQuatenary->q.arg1 || !nowQuatenary->info1.active) {//如果A,B是同一标识符或B以后不活跃
						Avalue[nowQuatenary->q.result].insert(*iter);
						Rvalue[*iter].insert(nowQuatenary->q.result);
						return *iter;
					}
				}
			}
		}
	}

	//为目标变量分配可能不正确
	//return allocateReg(nowQuatenary->q.des);
	string ret = allocateReg();
	Avalue[nowQuatenary->q.result].insert(ret);
	Rvalue[ret].insert(nowQuatenary->q.result);
	return ret;
}

//分析活跃变量信息
void ObjectCode::analyseBlock() {
	for (map<string, vector<Block> >::iterator fbiter = funcBlocks.begin(); fbiter != funcBlocks.end(); fbiter++) {
		vector<IBlock> iBlocks;
		vector<Block>& blocks = fbiter->second;
		vector<set<string> >OUTL = funcOUTL.at(fbiter->first);

		for (vector<Block>::iterator iter = blocks.begin(); iter != blocks.end(); iter++) {
			IBlock iBlock;
			iBlock.next1 = iter->next1;
			iBlock.next2 = iter->next2;
			iBlock.name = iter->name;
			for (vector<quadruple>::iterator qIter = iter->codes.begin(); qIter != iter->codes.end(); qIter++) {
				iBlock.codes.push_back(QuaternaryWithInfo(*qIter, VarInfomation(-1, false), VarInfomation(-1, false), VarInfomation(-1, false)));
			}
			iBlocks.push_back(iBlock);
		}

		vector<map<string, VarInfomation> > symTables;//每个基本块对应一张符号表
		//初始化符号表
		for (vector<Block>::iterator biter = blocks.begin(); biter != blocks.end(); biter++) {//遍历每一个基本块
			map<string, VarInfomation>symTable;
			for (vector<quadruple>::iterator citer = biter->codes.begin(); citer != biter->codes.end(); citer++) {//遍历基本块中的每个四元式
				if (citer->Op == Oper::J) {
					//pass
				}
				else if (citer->Op == Oper::Call)
				{
					if (isVar(citer->result)) {
						symTable[citer->result] = VarInfomation{ -1,false };
					}
				}
				else if (citer->Op == Oper::Jeq || citer->Op == Oper::Jge || citer->Op == Oper::Jgt || citer->Op == Oper::Jle
					|| citer->Op == Oper::Jlt || citer->Op == Oper::Jne)
				{
					if (isVar(citer->arg1)) {
						symTable[citer->arg1] = VarInfomation{ -1,false };
					}
					if (isVar(citer->arg2)) {
						symTable[citer->arg2] = VarInfomation{ -1,false };
					}
				}
				else if (citer->Op == Oper::Return)
				{
					if (isVar(citer->arg1)) {
						symTable[citer->arg1] = VarInfomation{ -1,false };
					}
				}
				else if (citer->Op == Oper::ArrayAssign)
				{
					if (isVar(citer->arg1)) {
						symTable[citer->arg1] = VarInfomation{ -1,false };
					}
					if (isVar(citer->result)) {
						symTable[citer->result] = VarInfomation{ -1,false };
					}
				}
				else if (citer->Op == Oper::AssignArray)
				{
					if (isVar(citer->arg2)) {
						symTable[citer->arg2] = VarInfomation{ -1,false };
					}
					if (isVar(citer->result)) {
						symTable[citer->result] = VarInfomation{ -1,false };
					}
				}
				else {
					if (isVar(citer->arg1)) {
						symTable[citer->arg1] = VarInfomation{ -1,false };
					}
					if (isVar(citer->arg2)) {
						symTable[citer->arg2] = VarInfomation{ -1,false };
					}
					if (isVar(citer->result)) {
						symTable[citer->result] = VarInfomation{ -1,false };
					}
				}
			}
			symTables.push_back(symTable);
		}
		int blockIndex = 0;
		for (vector<set<string> >::iterator iter = OUTL.begin(); iter != OUTL.end(); iter++, blockIndex++) {//遍历每个基本块的活跃变量表
			for (set<string>::iterator viter = iter->begin(); viter != iter->end(); viter++) {//遍历活跃变量表中的变量
				symTables[blockIndex][*viter] = VarInfomation{ -1,true };
			}

		}

		blockIndex = 0;
		//计算每个四元式的待用信息和活跃信息
		for (vector<IBlock>::iterator ibiter = iBlocks.begin(); ibiter != iBlocks.end(); ibiter++, blockIndex++) {//遍历每一个基本块
			int codeIndex = ibiter->codes.size() - 1;
			for (vector<QuaternaryWithInfo>::reverse_iterator citer = ibiter->codes.rbegin(); citer != ibiter->codes.rend(); citer++, codeIndex--) {//逆序遍历基本块中的代码
				if (citer->q.Op == Oper::J) {
					//pass
				}
				else if (citer->q.Op == Oper::Call)
				{
					if (isVar(citer->q.result)) {
						citer->info3 = symTables[blockIndex][citer->q.result];
						symTables[blockIndex][citer->q.result] = VarInfomation{ -1,false };
					}
				}
				else if (citer->q.Op == Oper::Jeq || citer->q.Op == Oper::Jge || citer->q.Op == Oper::Jgt || citer->q.Op == Oper::Jle
					|| citer->q.Op == Oper::Jlt || citer->q.Op == Oper::Jne)
				{
					if (isVar(citer->q.arg1)) {
						citer->info1 = symTables[blockIndex][citer->q.arg1];
						symTables[blockIndex][citer->q.arg1] = VarInfomation{ codeIndex,true };
					}
					if (isVar(citer->q.arg2)) {
						citer->info2 = symTables[blockIndex][citer->q.arg2];
						symTables[blockIndex][citer->q.arg2] = VarInfomation{ codeIndex,true };
					}
				}
				else if (citer->q.Op == Oper::Return)
				{
					if (isVar(citer->q.arg1)) {
						citer->info1 = symTables[blockIndex][citer->q.arg1];
						symTables[blockIndex][citer->q.arg1] = VarInfomation{ codeIndex,true };
					}
				}
				else if (citer->q.Op == Oper::ArrayAssign)
				{
					if (isVar(citer->q.arg1)) {
						citer->info1 = symTables[blockIndex][citer->q.arg1];
						symTables[blockIndex][citer->q.arg1] = VarInfomation{ codeIndex,true };
					}
					if (isVar(citer->q.result)) {
						citer->info3 = symTables[blockIndex][citer->q.result];
						symTables[blockIndex][citer->q.result] = VarInfomation{ -1,false };
					}
				}
				else if (citer->q.Op == Oper::AssignArray)
				{
					if (isVar(citer->q.arg2)) {
						citer->info2 = symTables[blockIndex][citer->q.arg2];
						symTables[blockIndex][citer->q.arg2] = VarInfomation{ codeIndex,true };
					}
					if (isVar(citer->q.result)) {
						citer->info3 = symTables[blockIndex][citer->q.result];
						symTables[blockIndex][citer->q.result] = VarInfomation{ -1,false };
					}
				}
				else {
					if (isVar(citer->q.arg1)) {
						citer->info1 = symTables[blockIndex][citer->q.arg1];
						symTables[blockIndex][citer->q.arg1] = VarInfomation{ codeIndex,true };
					}
					if (isVar(citer->q.arg2)) {
						citer->info2 = symTables[blockIndex][citer->q.arg2];
						symTables[blockIndex][citer->q.arg2] = VarInfomation{ codeIndex,true };
					}
					if (isVar(citer->q.result)) {
						citer->info3 = symTables[blockIndex][citer->q.result];
						symTables[blockIndex][citer->q.result] = VarInfomation{ -1,false };
					}
				}
			}
		}

		funcIBlocks[fbiter->first] = iBlocks;
	}
}

//输出信息变量基本块
void ObjectCode::outputIBlocks(ostream& out) {
	for (map<string, vector<IBlock> >::iterator iter = funcIBlocks.begin(); iter != funcIBlocks.end(); iter++) {
		out << "[" << iter->first << "]" << endl;
		for (vector<IBlock>::iterator bIter = iter->second.begin(); bIter != iter->second.end(); bIter++) {
			out << bIter->name << ":" << endl;
			for (vector<QuaternaryWithInfo>::iterator cIter = bIter->codes.begin(); cIter != bIter->codes.end(); cIter++) {
				out << "    ";
				cIter->output(out);
				out << endl;
			}
			out << "    " << "next1 = " << bIter->next1 << endl;
			out << "    " << "next2 = " << bIter->next2 << endl;
		}
		out << endl;
	}
}

//输出目标代码
void ObjectCode::outputObjectCode(ostream& out) {
	for (vector<string>::iterator iter = objectCodes.begin(); iter != objectCodes.end(); iter++) {
		out << *iter << endl;
	}
}

//基本块出口，将出口活跃变量保存在内存
void ObjectCode::storeOutLiveVar(set<string>& outl) {
	//遍历出口活跃变量集合
	for (set<string>::iterator oiter = outl.begin(); oiter != outl.end(); oiter++) {
		string reg;//活跃变量所在的寄存器名称
		bool inFlag = false;//活跃变量在内存中的标志
		//遍历活跃变量所有存取位置
		for (set<string>::iterator aiter = Avalue[*oiter].begin(); aiter != Avalue[*oiter].end(); aiter++) {
			if ((*aiter)[0] != '$') {//该活跃变量已经存储在内存中
				inFlag = true;
				break;
			}
			else reg = *aiter;
		}
		//如果该活跃变量不在内存中，则将reg中的var变量存入内存
		if (!inFlag) storeVar(reg, *oiter);
	}
}

//为基本块中的每一个四元式代码生成目标代码
void ObjectCode::generateCodeForQuatenary(int nowBaseBlockIndex, int& arg_num, int& par_num, list<pair<string, bool> >& par_list)
{
	//除了跳转和调用操作，其他操作需要检查变量是否在引用前已被赋值
	if (Oper2string(nowQuatenary->q.Op)[0] != 'J' && nowQuatenary->q.Op != Oper::Call) {
		//是变量且Avalue 中没有赋值记录
		if (isVar(nowQuatenary->q.arg1) && Avalue[nowQuatenary->q.arg1].empty()) {
			cout << string("变量") << nowQuatenary->q.arg1 << "在引用前未赋值" << endl;
		}
		if (isVar(nowQuatenary->q.arg2) && Avalue[nowQuatenary->q.arg2].empty()) {
			cout << string("变量") << nowQuatenary->q.arg2 << "在引用前未赋值" << endl;
		}
	}
	//无条件跳转 直接生成
	if (nowQuatenary->q.Op == Oper::J) {
		objectCodes.push_back(Oper2string(nowQuatenary->q.Op) + " " + nowQuatenary->q.result);
	}
	//条件跳转具体分析
	else if (Oper2string(nowQuatenary->q.Op)[0] == 'J')
	{
		//确定具体跳转指令
		string op;
		if (nowQuatenary->q.Op == Oper::Jge)op = "bge";
		else if (nowQuatenary->q.Op == Oper::Jgt)op = "bgt";
		else if (nowQuatenary->q.Op == Oper::Jeq)op = "beq";
		else if (nowQuatenary->q.Op == Oper::Jne)op = "bne";
		else if (nowQuatenary->q.Op == Oper::Jlt)op = "blt";
		else if (nowQuatenary->q.Op == Oper::Jle)op = "ble";
		string pos1 = allocateReg(nowQuatenary->q.arg1);
		string pos2 = allocateReg(nowQuatenary->q.arg2);
		objectCodes.push_back(op + " " + pos1 + " " + pos2 + " " + nowQuatenary->q.result);
		//如果不活跃，立即释放这两个变量的寄存器
		if (!nowQuatenary->info1.active) 
			releaseVar(nowQuatenary->q.arg1);
		if (!nowQuatenary->info2.active) 
			releaseVar(nowQuatenary->q.arg2);
	}
	//参数传递指令
	else if (nowQuatenary->q.Op == Oper::Parm) {
		//将参数和其活跃性状态加入到 par_list 中
		par_list.push_back(pair<string, bool>(nowQuatenary->q.arg1, nowQuatenary->info1.active));
	}
	//函数调用指令
	else if (nowQuatenary->q.Op == Oper::Call) {
		//寄存器压栈 保护现场
		int save_reg = 0;
		map<int, string>temp_save_reg;
		temp_save_reg.clear();
		for (map<string, set<string>>::iterator iter = Avalue.begin(); iter != Avalue.end(); ++iter) {
			for (set<string>::iterator setIt = iter->second.begin(); setIt != iter->second.end(); ++setIt) {
				if ((*setIt)[0] == '$' && (*setIt)[1] == 's') {
					objectCodes.push_back(string("sw ") + *setIt + " " + to_string(top + 4 * save_reg++) + "($sp)");
					temp_save_reg[save_reg] = *setIt;
					break;
				}
			}
		}
		top = top + 4 * save_reg;
		//将参数压栈
		for (list<pair<string, bool> >::iterator aiter = par_list.begin(); aiter != par_list.end(); aiter++) {
			string pos = allocateReg(aiter->first);//为形参分配寄存器
			//sp为形参预留栈帧空间
			objectCodes.push_back(string("sw ") + pos + " " + to_string(top + 4 * (++arg_num + 1)) + "($sp)");
			//不活跃，立即释放
			if (!aiter->second) releaseVar(aiter->first);
		}
		//更新$sp
		objectCodes.push_back(string("sw $sp ") + to_string(top) + "($sp)");//返回地址
		objectCodes.push_back(string("addi $sp $sp ") + to_string(top));//esp上移
		//跳转到对应函数
		objectCodes.push_back(string("jal ") + nowQuatenary->q.arg1);
		//恢复现场
		objectCodes.push_back(string("lw $sp 0($sp)"));//esp恢复
		//恢复寄存器
		top = top - 4 * save_reg;
		for (int i = save_reg; i >= 1; --i) {
			objectCodes.push_back(string("lw ") + temp_save_reg[i] + " " + to_string(top + 4 * (i - 1)) + "($sp)");
		}
		//保存返回值
		string src1Pos = "$v0";
		//返回值与其寄存器v0映射
		Rvalue[src1Pos].insert(nowQuatenary->q.result);
		Avalue[nowQuatenary->q.result].insert(src1Pos);
		par_list.clear();//实参列表清空
		arg_num = 0;//实参数量清零
	}
	//函数返回指令
	else if (nowQuatenary->q.Op == Oper::Return) {
		//cout << nowFunc <<" " << isNum(nowQuatenary->q.arg1) << " " << isVar(nowQuatenary->q.arg1) << endl;
		//返回值为数字
		if (isNum(nowQuatenary->q.arg1)) 
			objectCodes.push_back("addi $v0 $zero " + nowQuatenary->q.arg1);
		//返回值为变量
		else if (isVar(nowQuatenary->q.arg1)) {
			set<string>::iterator piter = Avalue[nowQuatenary->q.arg1].begin();
			//找到该变量的寄存器
			if ((*piter)[0] == '$') 
				objectCodes.push_back(string("add $v0 $zero ") + *piter);
			//否则从内存中找到该变量
			else 
				objectCodes.push_back(string("lw $v0 ") + to_string(varOffset[*piter]) + "($sp)");
		}
		//如果函数是main函数，还要结束
		if (nowFunc == "main") 
			objectCodes.push_back("j end");
		//否则返回到原函数调用指令的下一条指令
		else {
			objectCodes.push_back("lw $ra 4($sp)");
			objectCodes.push_back("jr $ra");
		}
	}
	//形参声明指令
	else if (nowQuatenary->q.Op == Oper::Get) {
		//varOffset[nowQuatenary->q.src1] = 4 * (par_num++ + 2);
		//相当于变量入栈
		varOffset[nowQuatenary->q.result] = top;
		//更新栈顶
		top += 4;
		Avalue[nowQuatenary->q.result].insert(nowQuatenary->q.result);
	}
	//赋值语句
	else if (nowQuatenary->q.Op == Oper::Assign) {
		string src1Pos;
		//为这个值分配寄存器
		src1Pos = allocateReg(nowQuatenary->q.arg1);
		//值与寄存器的映射
		Rvalue[src1Pos].insert(nowQuatenary->q.result);
		Avalue[nowQuatenary->q.result].insert(src1Pos);
	}
	//运算指令
	else {
		string src1Pos = allocateReg(nowQuatenary->q.arg1);
		string src2Pos = allocateReg(nowQuatenary->q.arg2);
		string desPos = getReg();//分配一个寄存器用于存运算结果
		if (nowQuatenary->q.Op == Oper::Plus) 
			objectCodes.push_back(string("add ") + desPos + " " + src1Pos + " " + src2Pos);
		else if (nowQuatenary->q.Op == Oper::Minus) 
			objectCodes.push_back(string("sub ") + desPos + " " + src1Pos + " " + src2Pos);
		else if (nowQuatenary->q.Op == Oper::Multiply) 
			objectCodes.push_back(string("mul ") + desPos + " " + src1Pos + " " + src2Pos);
		else if (nowQuatenary->q.Op == Oper::Divide) {
			objectCodes.push_back(string("div ") + src1Pos + " " + src2Pos);
			objectCodes.push_back(string("mflo ") + desPos);
		}
		//释放
		if (!nowQuatenary->info1.active) releaseVar(nowQuatenary->q.arg1);
		if (!nowQuatenary->info2.active) releaseVar(nowQuatenary->q.arg2);
	}
}

//对函数块中的每一个基本块生成目标代码
void ObjectCode::generateCodeForBaseBlocks(int nowBaseBlockIndex)
{
	int arg_num = 0;//par的实参个数
	int par_num = 0;//get的形参个数
	//函数调用时实参的列表
	list<pair<string, bool> > par_list;//函数调用用到的实参集list<实参名,是否活跃>
	Avalue.clear();//活跃变量的集合
	Rvalue.clear();//可用寄存器的集合
	//初始化当前nowFunc函数第nowBaseBlockIndex个基本块的入口变量活跃集
	set<string>& inl = funcINL[nowFunc][nowBaseBlockIndex];
	for (set<string>::iterator iter = inl.begin(); iter != inl.end(); iter++) {
		Avalue[*iter].insert(*iter);
	}
	//初始化8个空闲寄存器s0~s7
	freeReg.clear();
	for (int i = 0; i <= 7; i++) {
		freeReg.push_back(string("$s") + to_string(i));
	}
	objectCodes.push_back(nowIBlock->name + ":");
	if (nowBaseBlockIndex == 0) {//函数中的第一个基本块
		if (nowFunc == "main") {//且是在main函数中
			top = 8;
		}
		else {//其它函数
			objectCodes.push_back("sw $ra 4($sp)");//把返回地址压栈
			top = 8;
		}
	}
	//对基本块内的每一条语句
	for (vector<QuaternaryWithInfo>::iterator cIter = nowIBlock->codes.begin(); cIter != nowIBlock->codes.end(); cIter++) {
		nowQuatenary = cIter;//确定当前分析的四元式中间代码
		//如果是基本块的最后一条语句
		if (cIter + 1 == nowIBlock->codes.end()) {
			//如果最后一条语句是控制语句，则先将出口活跃变量保存，再进行跳转(j,call,return)
			if (isControlOp(Oper2string(cIter->q.Op))) {
				//if(Oper2string(cIter->q.Op)=="Return")
				storeOutLiveVar(funcOUTL[nowFunc][nowBaseBlockIndex]);
				generateCodeForQuatenary(nowBaseBlockIndex, arg_num, par_num, par_list);
			}
			//如果最后一条语句不是控制语句，则先计算，再将出口活跃变量保存
			else {
				generateCodeForQuatenary(nowBaseBlockIndex, arg_num, par_num, par_list);
				storeOutLiveVar(funcOUTL[nowFunc][nowBaseBlockIndex]);
			}
		}
		//不是基本块最后一条语句就不用考虑出口活跃变量
		else {
			generateCodeForQuatenary(nowBaseBlockIndex, arg_num, par_num, par_list);
		}
	}
}

//对每一个函数块生成目标代码
void ObjectCode::generateCodeForFuncBlocks(map<string, vector<IBlock> >::iterator& fiter)
{
	varOffset.clear();
	nowFunc = fiter->first;//当前函数块名字
	vector<IBlock>& iBlocks = fiter->second;//当前函数块的若干个基本块，数组
	//对每一个基本块生成目标代码
	for (vector<IBlock>::iterator iter = iBlocks.begin(); iter != iBlocks.end(); iter++) {
		nowIBlock = iter;
		generateCodeForBaseBlocks(nowIBlock - iBlocks.begin());//数组下标索引
	}
}

//生成数据段代码 针对数组
void ObjectCode::generateArrayData(const proc_symbolTable* ptr)
{
	objectCodes.push_back(".data");

	for (const auto& item : ptr->itemTable)	//全局数组
	{
		if (item.second.type == symbolType::Array)
		{
			objectCodes.push_back(item.second.gobalname + string(":"));
			int len = 1;
			for (const auto& arr : item.second.array)
				len *= arr;
			objectCodes.push_back(string(".word ") + to_string(len));
		}
	}

	for (const auto& func : ptr->functionTable)	//局部数组
	{
		for (const auto& item : func.second->itemTable)
		{
			if (item.second.type == symbolType::Array)
			{
				objectCodes.push_back(item.second.gobalname + string(":"));
				int len = 1;
				for (const auto& arr : item.second.array)
					len *= arr;
				objectCodes.push_back(string(".word ") + to_string(len));
			}
		}
	}
}

void ObjectCode::generateCode(const proc_symbolTable* ptr)
{
	this->generateArrayData(ptr);
	objectCodes.push_back(".text");
	objectCodes.push_back("lui $sp,0x1002");
	objectCodes.push_back("j main");
	//对每一个函数块生成目标代码
	for (map<string, vector<IBlock> >::iterator fiter = funcIBlocks.begin(); fiter != funcIBlocks.end(); fiter++) {
		generateCodeForFuncBlocks(fiter);
	}
	objectCodes.push_back("end:");
}