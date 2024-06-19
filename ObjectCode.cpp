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

	for (vector<pair<int, string> >::iterator iter = funcEnter.begin(); iter != funcEnter.end(); iter++) { //ÿһ��������
		vector<Block>blocks;
		priority_queue<int, vector<int>, greater<int> >block_enter; //���л���������λ��
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

		//���ֻ�����	
		Block block;
		map<int, string>labelEnter; //��ڵ�ͱ�ǩ�Ķ�Ӧ��ϵ
		map<int, int>enter_block; //������ڵ��block�Ķ�Ӧ��ϵ
		int firstFlag = true; //�������һ���ǣ��ÿ�����Ϊ������
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

			if (!firstFlag) { //�û����鲻�Ǻ�����ĵ�һ�������
				block.name = label.newLabel();
				labelEnter[lastEnter] = block.name;
			}
			else { //�û������Ǻ�����ĵ�һ�������
				block.name = iter->second;
				firstFlag = false;
			}

			enter_block[lastEnter] = blocks.size();
			blocks.push_back(block);
			lastEnter = enter;
			block.codes.clear();
		}
		if (!firstFlag) { //�û����鲻�Ǻ�����ĵ�һ�������
			block.name = label.newLabel();
			labelEnter[lastEnter] = block.name;
		}
		else { //�û������Ǻ�����ĵ�һ�������
			block.name = iter->second;
			firstFlag = false;
		}
		if (iter + 1 != funcEnter.end()) { //���������������֮��
			for (int i = lastEnter; i != (iter + 1)->first; i++) {
				block.codes.push_back(code.code[i - CODE_START_POS]);
			}
		}
		else {//�����һ���������м����ĩβ
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

		//��Ծ����������������
		for (vector<Block>::iterator biter = blocks.begin(); biter != blocks.end(); biter++) {
			set<string>def, use;
			for (vector<quadruple>::iterator citer = biter->codes.begin(); citer != biter->codes.end(); citer++) {
				if (citer->Op == Oper::ArrayAssign) {
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//���Դ������1��û�б���ֵ
						use.insert(citer->arg1);
					}
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//���Դ������2��û�б���ֵ
						use.insert(citer->arg2);
					}
					if (isVar(citer->result) && use.count(citer->result) == 0) {//���Ŀ�Ĳ�������û�б�����
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::Get) {
					if (isVar(citer->result) && use.count(citer->result) == 0) {//���Ŀ�Ĳ�������û�б�����
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::AssignArray) {
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//���Դ������2��û�б���ֵ
						use.insert(citer->arg2);
					}
					if (isVar(citer->result) && use.count(citer->result) == 0) {//���Ŀ�Ĳ�������û�б�����
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::Call) {
					if (isVar(citer->result) && use.count(citer->result) == 0) {//���Ŀ�Ĳ�������û�б�����
						def.insert(citer->result);
					}
				}
				else if (citer->Op == Oper::Parm || citer->Op == Oper::Return) {
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//���Դ������1��û�б���ֵ
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
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//���Դ������1��û�б���ֵ
						use.insert(citer->arg1);
					}
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//���Դ������2��û�б���ֵ
						use.insert(citer->arg2);
					}
				}
				else {
					if (isVar(citer->arg1) && def.count(citer->arg1) == 0) {//���Դ������1��û�б���ֵ
						use.insert(citer->arg1);
					}
					if (isVar(citer->arg2) && def.count(citer->arg2) == 0) {//���Դ������2��û�б���ֵ
						use.insert(citer->arg2);
					}
					if (isVar(citer->result) && use.count(citer->result) == 0) {//���Ŀ�Ĳ�������û�б�����
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

		//ȷ��INL��OUTL
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

//��reg�е�var���������ڴ�
void ObjectCode::storeVar(string reg, string var) {
	if (reg == "")return;
	if (varOffset.find(var) != varOffset.end()) {//����Ѿ�Ϊ*iter������˴洢�ռ�
		objectCodes.push_back(string("sw ") + reg + " " + to_string(varOffset[var]) + "($sp)");
	}
	else {
		varOffset[var] = top;
		top += 4;
		objectCodes.push_back(string("sw ") + reg + " " + to_string(varOffset[var]) + "($sp)");
	}
	Avalue[var].insert(var);
}

//�ͷżĴ���
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

//Ϊ���ñ�������Ĵ���
string ObjectCode::allocateReg() {
	//�������δ����ļĴ����������ѡȡһ��
	string ret;
	if (freeReg.size()) {
		ret = freeReg.back();
		freeReg.pop_back();
		return ret;
	}
	/*
	���ѷ���ļĴ�����ѡȡһ��RiΪ����Ҫ�ļĴ���R�����ʹ��Ri��������������
	ռ��Ri�ı�����ֵҲͬʱ����ڸñ��������浥Ԫ��
	�����ڻ�������Ҫ����Զ�Ľ����Ż����õ��򲻻����õ���
	*/
	const int inf = 1000000;
	int maxNextPos = 0;
	for (map<string, set<string> >::iterator iter = Rvalue.begin(); iter != Rvalue.end(); iter++) {//�������еļĴ���
		int nextpos = inf;
		for (set<string>::iterator viter = iter->second.begin(); viter != iter->second.end(); viter++) {//�����Ĵ����д���ı���
			bool inFlag = false;//�������������ط��洢�ı�־
			for (set<string>::iterator aiter = Avalue[*viter].begin(); aiter != Avalue[*viter].end(); aiter++) {//���������Ĵ洢λ��
				if (*aiter != iter->first) {//��������洢�������ط�
					inFlag = true;
					break;
				}
			}
			if (!inFlag) {//����������洢�ڼĴ����У��Ϳ�δ���ںδ������øñ���
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
		//��ret�ļĴ����б���ı���*iter�����Ƕ������ٴ洢��ret��
		Avalue[*iter].erase(ret);
		//���V�ĵ�ַ��������AVALUE[V]˵V��������R֮��������ط�������Ҫ���ɴ���ָ��
		if (Avalue[*iter].size() > 0) {
			//pass
		}
		//���V�����ڴ�֮��ʹ�ã�����Ҫ���ɴ���ָ��
		else {
			bool storeFlag = true;
			vector<QuaternaryWithInfo>::iterator cIter;
			for (cIter = nowQuatenary; cIter != nowIBlock->codes.end(); cIter++) {
				if (cIter->q.arg1 == *iter || cIter->q.arg2 == *iter) {//���V�ڱ��������б�����
					storeFlag = true;
					break;
				}
				if (cIter->q.result == *iter) {//���V�ڱ��������б���ֵ
					storeFlag = false;
					break;
				}
			}
			if (cIter == nowIBlock->codes.end()) {//���V�ڱ���������δ�����ã���Ҳû�б���ֵ
				int index = nowIBlock - funcIBlocks[nowFunc].begin();
				if (funcOUTL[nowFunc][index].count(*iter) == 1) {//����˱����ǳ���֮��Ļ�Ծ����
					storeFlag = true;
				}
				else {
					storeFlag = false;
				}
			}
			if (storeFlag) {//���ɴ���ָ��
				storeVar(ret, *iter);
			}
		}
	}
	Rvalue[ret].clear();//���ret�Ĵ����б���ı���

	return ret;
}

//Ϊ���ñ�������Ĵ���
string ObjectCode::allocateReg(string var) {
	//Ϊ��������Ĵ���
	if (isNum(var)) {
		string ret = allocateReg();//������мĴ���
		objectCodes.push_back(string("addi ") + ret + " $zero " + var);
		return ret;
	}
	for (set<string>::iterator iter = Avalue[var].begin(); iter != Avalue[var].end(); iter++) {
		if ((*iter)[0] == '$') {//��������Ѿ�������ĳ���Ĵ�����
			//if (var == "T18")cout << "&&&" << endl << *iter << endl;
			return *iter;//ֱ�ӷ��ظüĴ���
		}
	}
	//����ñ���û����ĳ���Ĵ�����
	string ret = allocateReg();
	objectCodes.push_back(string("lw ") + ret + " " + to_string(varOffset[var]) + "($sp)");
	Avalue[var].insert(ret);
	Rvalue[ret].insert(var);
	return ret;
}

//Ϊ����������Ĵ���
string ObjectCode::getReg() {
	//i: A:=B op C
	//���B������ֵ��ĳ���Ĵ���Ri�У�RVALUE[Ri]��ֻ����B
	//���⣬����B��A��ͬһ����ʶ������B������ֵ��ִ����ԪʽA:=B op C֮�󲻻�������
	//��ѡȡRiΪ����Ҫ�ļĴ���R

	//���src1��������
	if (!isNum(nowQuatenary->q.arg1)) {
		//����src1���ڵļĴ���
		set<string>& src1pos = Avalue[nowQuatenary->q.arg1];
		for (set<string>::iterator iter = src1pos.begin(); iter != src1pos.end(); iter++) {
			if ((*iter)[0] == '$') {
				if (Rvalue[*iter].size() == 1) {//����üĴ�����ֵ��������src1
					if (nowQuatenary->q.result == nowQuatenary->q.arg1 || !nowQuatenary->info1.active) {//���A,B��ͬһ��ʶ����B�Ժ󲻻�Ծ
						Avalue[nowQuatenary->q.result].insert(*iter);
						Rvalue[*iter].insert(nowQuatenary->q.result);
						return *iter;
					}
				}
			}
		}
	}

	//ΪĿ�����������ܲ���ȷ
	//return allocateReg(nowQuatenary->q.des);
	string ret = allocateReg();
	Avalue[nowQuatenary->q.result].insert(ret);
	Rvalue[ret].insert(nowQuatenary->q.result);
	return ret;
}

//������Ծ������Ϣ
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

		vector<map<string, VarInfomation> > symTables;//ÿ���������Ӧһ�ŷ��ű�
		//��ʼ�����ű�
		for (vector<Block>::iterator biter = blocks.begin(); biter != blocks.end(); biter++) {//����ÿһ��������
			map<string, VarInfomation>symTable;
			for (vector<quadruple>::iterator citer = biter->codes.begin(); citer != biter->codes.end(); citer++) {//�����������е�ÿ����Ԫʽ
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
		for (vector<set<string> >::iterator iter = OUTL.begin(); iter != OUTL.end(); iter++, blockIndex++) {//����ÿ��������Ļ�Ծ������
			for (set<string>::iterator viter = iter->begin(); viter != iter->end(); viter++) {//������Ծ�������еı���
				symTables[blockIndex][*viter] = VarInfomation{ -1,true };
			}

		}

		blockIndex = 0;
		//����ÿ����Ԫʽ�Ĵ�����Ϣ�ͻ�Ծ��Ϣ
		for (vector<IBlock>::iterator ibiter = iBlocks.begin(); ibiter != iBlocks.end(); ibiter++, blockIndex++) {//����ÿһ��������
			int codeIndex = ibiter->codes.size() - 1;
			for (vector<QuaternaryWithInfo>::reverse_iterator citer = ibiter->codes.rbegin(); citer != ibiter->codes.rend(); citer++, codeIndex--) {//��������������еĴ���
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

//�����Ϣ����������
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

//���Ŀ�����
void ObjectCode::outputObjectCode(ostream& out) {
	for (vector<string>::iterator iter = objectCodes.begin(); iter != objectCodes.end(); iter++) {
		out << *iter << endl;
	}
}

//��������ڣ������ڻ�Ծ�����������ڴ�
void ObjectCode::storeOutLiveVar(set<string>& outl) {
	//�������ڻ�Ծ��������
	for (set<string>::iterator oiter = outl.begin(); oiter != outl.end(); oiter++) {
		string reg;//��Ծ�������ڵļĴ�������
		bool inFlag = false;//��Ծ�������ڴ��еı�־
		//������Ծ�������д�ȡλ��
		for (set<string>::iterator aiter = Avalue[*oiter].begin(); aiter != Avalue[*oiter].end(); aiter++) {
			if ((*aiter)[0] != '$') {//�û�Ծ�����Ѿ��洢���ڴ���
				inFlag = true;
				break;
			}
			else reg = *aiter;
		}
		//����û�Ծ���������ڴ��У���reg�е�var���������ڴ�
		if (!inFlag) storeVar(reg, *oiter);
	}
}

//Ϊ�������е�ÿһ����Ԫʽ��������Ŀ�����
void ObjectCode::generateCodeForQuatenary(int nowBaseBlockIndex, int& arg_num, int& par_num, list<pair<string, bool> >& par_list)
{
	//������ת�͵��ò���������������Ҫ�������Ƿ�������ǰ�ѱ���ֵ
	if (Oper2string(nowQuatenary->q.Op)[0] != 'J' && nowQuatenary->q.Op != Oper::Call) {
		//�Ǳ�����Avalue ��û�и�ֵ��¼
		if (isVar(nowQuatenary->q.arg1) && Avalue[nowQuatenary->q.arg1].empty()) {
			cout << string("����") << nowQuatenary->q.arg1 << "������ǰδ��ֵ" << endl;
		}
		if (isVar(nowQuatenary->q.arg2) && Avalue[nowQuatenary->q.arg2].empty()) {
			cout << string("����") << nowQuatenary->q.arg2 << "������ǰδ��ֵ" << endl;
		}
	}
	//��������ת ֱ������
	if (nowQuatenary->q.Op == Oper::J) {
		objectCodes.push_back(Oper2string(nowQuatenary->q.Op) + " " + nowQuatenary->q.result);
	}
	//������ת�������
	else if (Oper2string(nowQuatenary->q.Op)[0] == 'J')
	{
		//ȷ��������תָ��
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
		//�������Ծ�������ͷ������������ļĴ���
		if (!nowQuatenary->info1.active) 
			releaseVar(nowQuatenary->q.arg1);
		if (!nowQuatenary->info2.active) 
			releaseVar(nowQuatenary->q.arg2);
	}
	//��������ָ��
	else if (nowQuatenary->q.Op == Oper::Parm) {
		//�����������Ծ��״̬���뵽 par_list ��
		par_list.push_back(pair<string, bool>(nowQuatenary->q.arg1, nowQuatenary->info1.active));
	}
	//��������ָ��
	else if (nowQuatenary->q.Op == Oper::Call) {
		//�Ĵ���ѹջ �����ֳ�
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
		//������ѹջ
		for (list<pair<string, bool> >::iterator aiter = par_list.begin(); aiter != par_list.end(); aiter++) {
			string pos = allocateReg(aiter->first);//Ϊ�βη���Ĵ���
			//spΪ�β�Ԥ��ջ֡�ռ�
			objectCodes.push_back(string("sw ") + pos + " " + to_string(top + 4 * (++arg_num + 1)) + "($sp)");
			//����Ծ�������ͷ�
			if (!aiter->second) releaseVar(aiter->first);
		}
		//����$sp
		objectCodes.push_back(string("sw $sp ") + to_string(top) + "($sp)");//���ص�ַ
		objectCodes.push_back(string("addi $sp $sp ") + to_string(top));//esp����
		//��ת����Ӧ����
		objectCodes.push_back(string("jal ") + nowQuatenary->q.arg1);
		//�ָ��ֳ�
		objectCodes.push_back(string("lw $sp 0($sp)"));//esp�ָ�
		//�ָ��Ĵ���
		top = top - 4 * save_reg;
		for (int i = save_reg; i >= 1; --i) {
			objectCodes.push_back(string("lw ") + temp_save_reg[i] + " " + to_string(top + 4 * (i - 1)) + "($sp)");
		}
		//���淵��ֵ
		string src1Pos = "$v0";
		//����ֵ����Ĵ���v0ӳ��
		Rvalue[src1Pos].insert(nowQuatenary->q.result);
		Avalue[nowQuatenary->q.result].insert(src1Pos);
		par_list.clear();//ʵ���б����
		arg_num = 0;//ʵ����������
	}
	//��������ָ��
	else if (nowQuatenary->q.Op == Oper::Return) {
		//cout << nowFunc <<" " << isNum(nowQuatenary->q.arg1) << " " << isVar(nowQuatenary->q.arg1) << endl;
		//����ֵΪ����
		if (isNum(nowQuatenary->q.arg1)) 
			objectCodes.push_back("addi $v0 $zero " + nowQuatenary->q.arg1);
		//����ֵΪ����
		else if (isVar(nowQuatenary->q.arg1)) {
			set<string>::iterator piter = Avalue[nowQuatenary->q.arg1].begin();
			//�ҵ��ñ����ļĴ���
			if ((*piter)[0] == '$') 
				objectCodes.push_back(string("add $v0 $zero ") + *piter);
			//������ڴ����ҵ��ñ���
			else 
				objectCodes.push_back(string("lw $v0 ") + to_string(varOffset[*piter]) + "($sp)");
		}
		//���������main��������Ҫ����
		if (nowFunc == "main") 
			objectCodes.push_back("j end");
		//���򷵻ص�ԭ��������ָ�����һ��ָ��
		else {
			objectCodes.push_back("lw $ra 4($sp)");
			objectCodes.push_back("jr $ra");
		}
	}
	//�β�����ָ��
	else if (nowQuatenary->q.Op == Oper::Get) {
		//varOffset[nowQuatenary->q.src1] = 4 * (par_num++ + 2);
		//�൱�ڱ�����ջ
		varOffset[nowQuatenary->q.result] = top;
		//����ջ��
		top += 4;
		Avalue[nowQuatenary->q.result].insert(nowQuatenary->q.result);
	}
	//��ֵ���
	else if (nowQuatenary->q.Op == Oper::Assign) {
		string src1Pos;
		//Ϊ���ֵ����Ĵ���
		src1Pos = allocateReg(nowQuatenary->q.arg1);
		//ֵ��Ĵ�����ӳ��
		Rvalue[src1Pos].insert(nowQuatenary->q.result);
		Avalue[nowQuatenary->q.result].insert(src1Pos);
	}
	//����ָ��
	else {
		string src1Pos = allocateReg(nowQuatenary->q.arg1);
		string src2Pos = allocateReg(nowQuatenary->q.arg2);
		string desPos = getReg();//����һ���Ĵ������ڴ�������
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
		//�ͷ�
		if (!nowQuatenary->info1.active) releaseVar(nowQuatenary->q.arg1);
		if (!nowQuatenary->info2.active) releaseVar(nowQuatenary->q.arg2);
	}
}

//�Ժ������е�ÿһ������������Ŀ�����
void ObjectCode::generateCodeForBaseBlocks(int nowBaseBlockIndex)
{
	int arg_num = 0;//par��ʵ�θ���
	int par_num = 0;//get���βθ���
	//��������ʱʵ�ε��б�
	list<pair<string, bool> > par_list;//���������õ���ʵ�μ�list<ʵ����,�Ƿ��Ծ>
	Avalue.clear();//��Ծ�����ļ���
	Rvalue.clear();//���üĴ����ļ���
	//��ʼ����ǰnowFunc������nowBaseBlockIndex�����������ڱ�����Ծ��
	set<string>& inl = funcINL[nowFunc][nowBaseBlockIndex];
	for (set<string>::iterator iter = inl.begin(); iter != inl.end(); iter++) {
		Avalue[*iter].insert(*iter);
	}
	//��ʼ��8�����мĴ���s0~s7
	freeReg.clear();
	for (int i = 0; i <= 7; i++) {
		freeReg.push_back(string("$s") + to_string(i));
	}
	objectCodes.push_back(nowIBlock->name + ":");
	if (nowBaseBlockIndex == 0) {//�����еĵ�һ��������
		if (nowFunc == "main") {//������main������
			top = 8;
		}
		else {//��������
			objectCodes.push_back("sw $ra 4($sp)");//�ѷ��ص�ַѹջ
			top = 8;
		}
	}
	//�Ի������ڵ�ÿһ�����
	for (vector<QuaternaryWithInfo>::iterator cIter = nowIBlock->codes.begin(); cIter != nowIBlock->codes.end(); cIter++) {
		nowQuatenary = cIter;//ȷ����ǰ��������Ԫʽ�м����
		//����ǻ���������һ�����
		if (cIter + 1 == nowIBlock->codes.end()) {
			//������һ������ǿ�����䣬���Ƚ����ڻ�Ծ�������棬�ٽ�����ת(j,call,return)
			if (isControlOp(Oper2string(cIter->q.Op))) {
				//if(Oper2string(cIter->q.Op)=="Return")
				storeOutLiveVar(funcOUTL[nowFunc][nowBaseBlockIndex]);
				generateCodeForQuatenary(nowBaseBlockIndex, arg_num, par_num, par_list);
			}
			//������һ����䲻�ǿ�����䣬���ȼ��㣬�ٽ����ڻ�Ծ��������
			else {
				generateCodeForQuatenary(nowBaseBlockIndex, arg_num, par_num, par_list);
				storeOutLiveVar(funcOUTL[nowFunc][nowBaseBlockIndex]);
			}
		}
		//���ǻ��������һ�����Ͳ��ÿ��ǳ��ڻ�Ծ����
		else {
			generateCodeForQuatenary(nowBaseBlockIndex, arg_num, par_num, par_list);
		}
	}
}

//��ÿһ������������Ŀ�����
void ObjectCode::generateCodeForFuncBlocks(map<string, vector<IBlock> >::iterator& fiter)
{
	varOffset.clear();
	nowFunc = fiter->first;//��ǰ����������
	vector<IBlock>& iBlocks = fiter->second;//��ǰ����������ɸ������飬����
	//��ÿһ������������Ŀ�����
	for (vector<IBlock>::iterator iter = iBlocks.begin(); iter != iBlocks.end(); iter++) {
		nowIBlock = iter;
		generateCodeForBaseBlocks(nowIBlock - iBlocks.begin());//�����±�����
	}
}

//�������ݶδ��� �������
void ObjectCode::generateArrayData(const proc_symbolTable* ptr)
{
	objectCodes.push_back(".data");

	for (const auto& item : ptr->itemTable)	//ȫ������
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

	for (const auto& func : ptr->functionTable)	//�ֲ�����
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
	//��ÿһ������������Ŀ�����
	for (map<string, vector<IBlock> >::iterator fiter = funcIBlocks.begin(); fiter != funcIBlocks.end(); fiter++) {
		generateCodeForFuncBlocks(fiter);
	}
	objectCodes.push_back("end:");
}