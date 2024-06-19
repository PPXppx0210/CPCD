#pragma once
#include "QuadCode.h"
#include "Parsing.h"

class NewLabeler {
private:
	int index;
	int tmp;
public:
	NewLabeler();
	string newLabel();
	string newTmp();
};

//������Ϣ
class VarInfomation {
public:
	int next;//������Ϣ
	bool active;//��Ծ��Ϣ

	VarInfomation(int next, bool active);
	VarInfomation(const VarInfomation& other);
	VarInfomation();
	void output(ostream& out);
};

//���б�����Ϣ����Ԫʽ
class QuaternaryWithInfo {
public:
	quadruple q;//��Ԫʽ
	VarInfomation info1;//������Ϣ
	VarInfomation info2;
	VarInfomation info3;

	QuaternaryWithInfo(quadruple q, VarInfomation info1, VarInfomation info2, VarInfomation info3);
	void output(ostream& out);
};

//���б�����Ϣ�Ļ�����
struct IBlock {
	string name;
	vector<QuaternaryWithInfo> codes;//���б�����Ϣ����Ԫʽ����
	int next1;
	int next2;
};

class ObjectCode {
private:
	IntermediateLanguage code;//�м����
	map<string, vector<Block> >funcBlocks; //�������Ļ�����
	map<string, vector<set<string> > >funcOUTL; //���������л�����ĳ��ڻ�Ծ������
	map<string, vector<set<string> > >funcINL; //���������л��������ڻ�Ծ������
	NewLabeler label;
	map<string, vector<IBlock> >funcIBlocks;//һ������ӳ�䵽���������
	map<string, set<string> >Avalue;//�洢�����еĻ�Ծ�����ļ���
	map<string, set<string> >Rvalue;//�洢�����еĿ��üĴ����ļ���
	map<string, int>varOffset;//�������Ĵ洢λ��
	int top;//��ǰջ��
	list<string>freeReg;//���еļĴ������
	string nowFunc;//��ǰ�����ĺ���
	vector<IBlock>::iterator nowIBlock;//��ǰ�����Ļ�����
	vector<QuaternaryWithInfo>::iterator nowQuatenary;//��ǰ��������Ԫʽ
	vector<string>objectCodes;//Ŀ�����

	void storeVar(string reg, string var);//�洢�����ͼĴ���֮���ӳ���ϵ
	void storeOutLiveVar(set<string>& outl);//�洢���ڻ�Ծ����
	void releaseVar(string var);//�ͷű����ļĴ���
	string getReg();//��ȡһ�����õļĴ���
	string allocateReg();//����һ���Ĵ���
	string allocateReg(string var);//Ϊ�ض���������һ���Ĵ���

	void generateArrayData(const proc_symbolTable*);//������������
	void generateCodeForFuncBlocks(map<string, vector<IBlock> >::iterator& fiter);//Ϊ�����ĸ�������������Ŀ�����
	void generateCodeForBaseBlocks(int nowBaseBlockIndex);//Ϊ����������Ŀ�����
	void generateCodeForQuatenary(int nowBaseBlockIndex, int& arg_num, int& par_num, list<pair<string, bool> >& par_list);//Ϊ��Ԫʽ����Ŀ�����
public:
	ObjectCode();
	void outputBlocks(ostream& out);  //���������
	void divideBlocks(const Parsing&, const proc_symbolTable*);  //�����﷨���ͷ��ű��ֻ�����
	void init_INOUTL(); //��ʼ������ڻ�Ծ������
	void generateCode(const proc_symbolTable*);//����Ŀ�����
	void analyseBlock();//����������õ�������Ϣ
	void outputIBlocks(ostream& out);//������������� ��������Ϣ
	void outputObjectCode(ostream& out);//���Ŀ�����
};