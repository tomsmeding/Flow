#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

using namespace std;

class ASTnode;
#if 0
class Variable;
#endif
class Token;
class Statement;
class Program;

typedef enum{WORD,NUMBER,STRING,SYMBOL,BLOCK}tokentype_t;
typedef enum{ROOT,ASSIGN,ASSIGN_DEST,LOOP,LOOP_BLOCK,LOOP_TYPE,CONDITION,EXPR,EXPR_NUM,EXPR_STR,EXPR_WORD,EXPR_OPER}astnodetype_t;

int prog_list_statements_depth=0,lexer_line_number=1;

void trim(string &s){
	size_t p=s.find_first_not_of(" \t\n");
	s.erase(0,p);
	p=s.find_last_not_of(" \t\n");
	if(p!=string::npos)s.erase(p+1);
}
int getUniqueInt(void){
	static  int Superman=0;
	   return   Superman++;
}
string intToString(int i){
	stringstream ss;
	ss<<i;
	return ss.str();
}
int getOperatorPrecedence(string &op){
	if(op=="!")return 1;
	if(op=="*"||op=="/")return 2;
	if(op=="+"||op=="-")return 3;
	if(op==">"||op=="<"||op==">="||op=="<=")return 4;
	if(op=="="||op=="!=")return 5;
	if(op=="&&"||op=="||"||op=="^^"||op=="!&&"||op=="!||"||op=="!^^")return 6;
	if(op=="not")return 7;
	if(op=="and"||op=="or"||op=="xor"||op=="nand"||op=="nor"||op=="nxor")return 8;
	return -1;
}
template <typename T>
bool vector_contains(const vector<T> &v,const T &t){
	for(T i:v)if(t==i)return true;
	return false;
}

class ASTnode{
public:
	astnodetype_t type;
	ASTnode *firstchild,*lastchild,*prevsibling,*nextsibling; //form a [linked list]-y thingy
	string *str;
	int id;
	ASTnode(ASTnode const&)=default;
	ASTnode(astnodetype_t);
	~ASTnode(void);
	void attachChild(ASTnode*);
	void attachSibling(ASTnode*);
	void printDOTrecur(stringstream*,int);
	string printDOT(void);
	static string getTypeString(int);
	ASTnode* getChildByType(astnodetype_t);
	string compile(int);
};

#if 0
class Variable{
	double numval;
	string *strval;
	bool numvalid,strvalid;
public:
	Variable(void);
	~Variable(void);
	double getnum(void);
	string* getstr(void);
	void setnum(double);
	void setstr(string&);
};
#endif

class Token{
public:
	string *tknstr;
	Program *blockprog;
	tokentype_t type;
	int linenum;
	Token(tokentype_t,string,int);
	Token(const Token&);
	~Token(void);
	string getString(void);
};

class Statement{
public:
	vector<Token*> *tkns;
	Statement(void);
	Statement(const Statement&);
	~Statement(void);
	int readFromString(string&,int);
	string getString(void);
};

class Program{
public:
	vector<Statement*> *stmts;
	Program(string);
	Program(const Program&);
	~Program(void);
	string listStatements(void);
	void buildAST(ASTnode*);
	string compile(ASTnode*);
	static void buildExpressionAST(ASTnode*,vector<Token*>*,int,int);
};

ASTnode::ASTnode(astnodetype_t _t){
	type=_t;
	firstchild=lastchild=prevsibling=nextsibling=NULL;
	str=new string();
	id=getUniqueInt();
}
ASTnode::~ASTnode(void){
	ASTnode *node,*node2;
	if(firstchild!=NULL){
		for(node=firstchild;node!=NULL;){
			node2=node->nextsibling;
			delete node;
			node=node2;
		}
	}
	if(str!=NULL)delete str;
}
void ASTnode::attachChild(ASTnode *node){
	if(node==NULL)cerr<<"ASTnode::attachChild(NULL)!"<<endl;
	if(firstchild==NULL){
		firstchild=lastchild=node;
	} else {
		lastchild->nextsibling=node;
		node->prevsibling=lastchild;
		lastchild=node;
	}
}
void ASTnode::printDOTrecur(stringstream *ss,int tablevel){
	ASTnode *node;
	int i;
	string typestr;
	typestr=ASTnode::getTypeString(type);
	for(node=firstchild;node!=NULL;node=node->nextsibling){
		for(i=0;i<tablevel;i++)*ss<<"\t";
		*ss<<"\""<<typestr<<"("<<id<<")\\n"<<*str<<"\"->\""<<ASTnode::getTypeString(node->type)<<"("<<node->id<<")\\n"<<*node->str<<"\";\n";
		node->printDOTrecur(ss,tablevel+1);
	}
}
string ASTnode::printDOT(void){
	stringstream ss;
	ss<<"digraph G"<<id<<"{\n";
	printDOTrecur(&ss,1);
	ss<<"}\n";
	return ss.str();
}
string ASTnode::getTypeString(int _t){
	string s;
	switch(_t){
	case ROOT:s="[ROOT]";break;
	case ASSIGN:s="[ASSIGN]";break;
	case ASSIGN_DEST:s="[ASSIGN_DEST]";break;
	case LOOP:s="[LOOP]";break;
	case LOOP_BLOCK:s="[LOOP_BLOCK]";break;
	case LOOP_TYPE:s="[LOOP_TYPE]";break;
	case CONDITION:s="[CONDITION]";break;
	case EXPR:s="[EXPR]";break;
	case EXPR_NUM:s="[EXPR_NUM]";break;
	case EXPR_STR:s="[EXPR_STR]";break;
	case EXPR_WORD:s="[EXPR_WORD]";break;
	case EXPR_OPER:s="[EXPR_OPER]";break;
	default:s="[UNKNOWN]";break;
	}
	return s;
}
ASTnode* ASTnode::getChildByType(astnodetype_t _t){
	ASTnode *node;
	for(node=firstchild;node!=NULL;node=node->nextsibling)if(node->type==_t)return node;
	return NULL;
}
void compile_get_all_variables(ASTnode *root,vector<string> &acc){
	ASTnode *n;
	if((root->type==ASSIGN_DEST||root->type==EXPR_WORD)&&!vector_contains(acc,*root->str))acc.push_back(*root->str);
	for(n=root->firstchild;n!=NULL;n=n->nextsibling){
		compile_get_all_variables(n,acc);
	}
}
string ASTnode::compile(int tablevel){
	int i;
	ASTnode *node;
	stringstream ss;
	switch(type){
	case ROOT:
	{
		vector<string> variables;
		ss<<
"#include <stdio.h>\n"
"#include <string>\n"
"\n"
"using namespace std;\n"
"\n"
"class Var{\n"
"	double _dbl; bool dblvalid;\n"
"	string _str; bool strvalid;\n"
"public:\n"
"	Var(void){dblvalid=strvalid=true;_dbl=0;_str=\"0\";}\n"
"	bool valid_double(void){return dblvalid;}\n"
"	bool valid_string(void){return dblvalid;}\n"
"	operator double(){\n"
"		if(dblvalid)return _dbl;\n"
"		if(strvalid){\n"
"			dblvalid=true;\n"
"			return _dbl=strtod(_str.c_str(),NULL);\n"
"		} else throw \"No valid value in Var!\";\n"
"	}\n"
"	operator string(){\n"
"		if(strvalid)return _str;\n"
"		if(dblvalid){\n"
"			strvalid=true;\n"
"			return _str=to_string(_dbl);\n"
"		} else throw \"No valid value in Var!\";\n"
"	}\n"
"	Var& operator=(const double other){\n"
"		strvalid=false;\n"
"		dblvalid=true;\n"
"		_dbl=other;\n"
"		return *this;\n"
"	}\n"
"	Var& operator=(const string &other){\n"
"		dblvalid=false;\n"
"		strvalid=true;\n"
"		_str=other;\n"
"		return *this;\n"
"	}\n"
"\n"
"};\n"
"\n"
"void print(const string &s){printf(\"%s\",s.c_str());}\n"
"void print(const char *s){printf(\"%s\",s);}\n"
"void print(const double d){printf(\"%g\",d);}\n"
"void print(Var &v){\n"
"	if(v.valid_double())printf(\"%g\",(double)v);\n"
"	else printf(\"%s\",((string)v).c_str());\n"
"}\n"
"\n"
"int main(int argc,char **argv){\n";
		compile_get_all_variables(this,variables);
		for(string varname : variables){
			ss<<"\tVar var_"<<varname<<";"<<endl;
		}
		for(node=firstchild;node!=NULL;node=node->nextsibling){
			ss<<node->compile(tablevel+1);
		}
		ss<<"\treturn 0;\n}\n"<<endl;
		break;
	}
	case ASSIGN:
		for(i=0;i<tablevel;i++)ss<<'\t';
		node=getChildByType(ASSIGN_DEST);
		if(*node->str=="out"){
			ss<<"print("<<getChildByType(EXPR)->compile(tablevel)<<");"<<endl;
		} else {
			ss<<node->compile(tablevel)<<"="<<getChildByType(EXPR)->compile(tablevel)<<";"<<endl;
		}
		break;
	case ASSIGN_DEST:
		ss<<"var_"<<*str;
		break;
	case LOOP:
		for(i=0;i<tablevel;i++)ss<<'\t';
		node=getChildByType(LOOP_TYPE);
		ss<<"do{"<<endl;
		ss<<getChildByType(LOOP_BLOCK)->compile(tablevel);
		for(i=0;i<tablevel;i++)ss<<'\t';
		ss<<"}while(";
		if(*node->str=="while"){
			ss<<getChildByType(CONDITION)->compile(tablevel);
		} else if(*node->str=="until"){
			ss<<"!("<<getChildByType(CONDITION)->compile(tablevel)<<")";
		} else {
			cerr<<"Unrecognised loop type '"<<*node->str<<"'!"<<endl;
			exit(1);
		}
		ss<<");"<<endl;
		break;
	case LOOP_BLOCK:
		for(node=firstchild;node!=NULL;node=node->nextsibling){
			ss<<node->compile(tablevel+1);
		}
		break;
	case EXPR:case CONDITION:
		return firstchild->compile(tablevel);
	case EXPR_NUM:
		ss<<*str;
		break;
	case EXPR_STR:
		ss<<'"';
		for(char c : *str){
			if(c=='\n')ss<<"\\n";
			else if(c=='\r')ss<<"\\r";
			else if(c=='\\')ss<<"\\\\";
			else ss<<c;
		}
		ss<<'"';
		break;
	case EXPR_WORD:
		ss<<"var_"<<*str;
		break;
	case EXPR_OPER:
		if(firstchild==NULL)
			ss<<*str;
		else if(firstchild==lastchild)
			ss<<*str<<"("<<firstchild->compile(tablevel)<<")";
		else if(firstchild->nextsibling==lastchild)
			ss<<"("<<firstchild->compile(tablevel)<<")"<<(*str=="="?"==":str->c_str())<<"("<<lastchild->compile(tablevel)<<")";
		else {
			cerr<<"Unsupported number of operands to operator '"<<*str<<"'! (ASTnode->id="<<id<<")"<<endl;
			exit(1);
		}
		break;
	default:
		cerr<<"Unrecognised statement type "<<ASTnode::getTypeString(type)<<"!"<<endl;
		break;
	}
	return ss.str();
}

#if 0
Variable::Variable(void){
	strval=new string;
	numvalid=false;
	strvalid=false;
}
Variable::~Variable(void){
	delete strval;
}
double Variable::getnum(void){
	const char *startptr;
	char *endptr;
	if(!numvalid){
		if(!strvalid){
			cerr<<"Variable "<<this<<" does not have a valid state, number requested! (numval="<<numval<<" strval="<<flush<<*strval<<")"<<endl;
			exit(1);
		}
		startptr=strval->c_str();
		numval=strtod(startptr,&endptr);
		if(endptr==startptr)numval=0;
	}
	return numval;
}
string* Variable::getstr(void){
	if(!strvalid){
		if(!numvalid){
			cerr<<"Variable "<<this<<" does not have a valid state, string requested! (numval="<<numval<<" strval="<<flush<<*strval<<")"<<endl;
			exit(1);
		}
		stringstream *ss=new stringstream;
		*ss<<numval;
		*strval=ss->str();
		delete ss;
	strvalid=false;
}
void Variable::setnum(double _n){
	numval=_n;
	numvalid=true;
	}
	return strval;
}
void Variable::setstr(string &s){
	*strval=s;
	strvalid=true;
	numvalid=false;
}
#endif

Token::Token(tokentype_t _type,string _tkn,int _lnum){
	type=_type;
	tknstr=new string(_tkn);
	linenum=_lnum;
	//cerr<<"new Token("<<type<<",\""<<*tknstr<<"\")"<<endl;
}
Token::Token(const Token &other){
	if(other.tknstr!=NULL)tknstr=new string(*other.tknstr);
	if(other.blockprog!=NULL)blockprog=new Program(*other.blockprog);
	type=other.type;
	linenum=other.linenum;
}
Token::~Token(void){
	delete tknstr;
	if(type==BLOCK)delete blockprog;
}
string Token::getString(void){
	stringstream *ss=new stringstream;
	switch(type){
	case WORD:*ss<<"[word: "<<*tknstr<<"]";break;
	case NUMBER:*ss<<"[number: "<<*tknstr<<"]";break;
	case STRING:*ss<<"[string: "<<*tknstr<<"]";break;
	case SYMBOL:*ss<<"[symbol: "<<*tknstr<<"]";break;
	case BLOCK:*ss<<"[block: "<<blockprog->listStatements()<<"]";break;
	default:*ss<<"[unkwown("<<type<<"): "<<*tknstr<<"]";break;
	}
	string ans=ss->str();
	delete ss;
	return ans;
}

Statement::Statement(void){
	//str=new string;
	tkns=new vector<Token*>;
}
Statement::Statement(const Statement &other){
	tkns=new vector<Token*>(*other.tkns);
}
Statement::~Statement(void){
	//delete str;
	delete tkns;
}
int Statement::readFromString(string &s,int from){
	size_t cursor,i;
	char c;
	bool done=false,dot,justDidOperator;
	//cerr<<"Statement::readFromString(\""<<s<<"\","<<from<<")"<<endl;
	justDidOperator=false;
	for(cursor=from;!done;){
		if(cursor>=s.size()){cursor=string::npos;break;}
		cursor=s.find_first_not_of(" \t",cursor);
		if(cursor==string::npos||cursor>=s.size()){cursor=string::npos;break;}
		c=s[cursor];
		switch(c){
		case '\n':lexer_line_number++;cursor++;break;
		case '(':case ')':case '+':case '*':case '/':case '=':
			tkns->push_back(new Token(SYMBOL,s.substr(cursor,1),lexer_line_number));
			cursor++;
			justDidOperator=true;
			break;
		case '-':
			if(cursor+1<s.size()&&s[cursor+1]=='>'){
				if(cursor+2<s.size()&&s[cursor+2]=='>'){
					tkns->push_back(new Token(SYMBOL,s.substr(cursor,3),lexer_line_number));
					cursor+=3;
				} else {
					tkns->push_back(new Token(SYMBOL,s.substr(cursor,2),lexer_line_number));
					cursor+=2;
				}
				justDidOperator=true;
			} else if(justDidOperator&&cursor+1<s.size()&&((s[cursor+1]>='0'&&s[cursor+1]<='9')||(cursor+2<s.size()&&s[cursor+1]=='.'))){
				c=s[cursor+1];
				dot=false;
				if(c=='.')dot=true;
				for(i=(c=='.'?cursor+2:cursor+1);i<s.size();i++){
					c=s[i];
					if(c<'0'||c>'9'){
						if(c=='.'&&!dot)dot=true;
						else break;
					}
				}
				tkns->push_back(new Token(NUMBER,s.substr(cursor,i-cursor),lexer_line_number));
				cursor=i;
				justDidOperator=false;
			} else {
				tkns->push_back(new Token(SYMBOL,s.substr(cursor,1),lexer_line_number));
				cursor++;
				justDidOperator=true;
			}
			break;
		case '!':
			if(cursor+2<s.size()&&(s[cursor+1]=='&'||s[cursor+1]=='|'||s[cursor+1]=='^')&&s[cursor+2]==s[cursor+1]){
				tkns->push_back(new Token(SYMBOL,s.substr(cursor,3),lexer_line_number));
				cursor+=3;
			} else if(cursor+1<s.size()&&s[cursor+1]=='='){
				tkns->push_back(new Token(SYMBOL,s.substr(cursor,2),lexer_line_number));
				cursor+=2;
			} else {
				tkns->push_back(new Token(SYMBOL,s.substr(cursor,1),lexer_line_number));
				cursor++;
			}
			justDidOperator=true;
			break;
		case '>':case '<':
			if(cursor+1<s.size()&&s[cursor+1]=='='){
				tkns->push_back(new Token(SYMBOL,s.substr(cursor,2),lexer_line_number));
				cursor+=2;
			} else {
				tkns->push_back(new Token(SYMBOL,s.substr(cursor,1),lexer_line_number));
				cursor++;
			}
			justDidOperator=true;
			break;
		case '&':case '|':case '^':
			if(cursor+1<s.size()&&s[cursor+1]==c){
				tkns->push_back(new Token(SYMBOL,s.substr(cursor,2),lexer_line_number));
				cursor+=2;
			} else {
				cerr<<":"<<lexer_line_number<<":Unknown token '"<<c<<"'"<<endl;
				exit(1);
			}
			justDidOperator=true;
			break;
		case ';':
			tkns->push_back(new Token(SYMBOL,s.substr(cursor,1),lexer_line_number));
			cursor++;
			done=true;
			break;
		case '{':
		{
			int level;
			for(i=cursor+1,level=1;i<s.size()&&level>0;i++){
				if(s[i]=='{')level++;
				if(s[i]=='}')level--;
			}
			if(i==s.size()){
				cerr<<":"<<lexer_line_number<<":Unclosed block"<<endl;
				exit(1);
			}
			i--;
			Token *tkn=new Token(BLOCK,s.substr(cursor+1,i-cursor-1),lexer_line_number);
			tkn->blockprog=new Program(*(tkn->tknstr));
			tkns->push_back(tkn);
			cursor=i+1;
			justDidOperator=true;
			break;
		}
		case '}':
			cerr<<":"<<lexer_line_number<<":Stray '}' in program"<<endl;
			exit(1);
		case '"':
		{
			string *contents=new string;
			for(i=cursor+1;i<s.size();i++){
				if(s[i]=='"')break;
				else if(s[i]=='\\'){
					if(i+1>=s.size()){
						cerr<<":"<<lexer_line_number<<":Unterminated string escape at end of code"<<endl;
						exit(1);
					}
					i++;
					switch(s[i]){
					case '\\':contents->push_back('\\');break;
					case '"':contents->push_back('"');break;
					case 'n':contents->push_back('\n');break;
					case 'r':contents->push_back('\r');break;
					case 't':contents->push_back('\t');break;
					default:contents->push_back(s[i]);cerr<<":"<<lexer_line_number<<":Warning: unrecognised escape sequence \"\\"<<s[i]<<"\""<<endl;
					}
				} else {
					if(s[i]=='\n')lexer_line_number++;
					contents->push_back(s[i]);
				}
			}
			if(s[i]!='"'){
				cerr<<":"<<lexer_line_number<<":Unterminated string literal at end of code: \""<<s.substr(cursor+1,i-cursor+1)<<"\""<<endl;
				exit(1);
			}
			cursor=i+1;
			tkns->push_back(new Token(STRING,*contents,lexer_line_number));
			delete contents;
			justDidOperator=false;
			break;
		}
		default:
			c=s[cursor];
			if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_'){
				for(i=cursor;i<s.size();i++){
					c=s[i];
					if((c<'A'||c>'Z')&&(c<'a'||c>'z')&&(c<'0'||c>'9')&&c!='_')break;
				}
				tkns->push_back(new Token(WORD,s.substr(cursor,i-cursor),lexer_line_number));
				cursor=i;
			} else if((c>='0'&&c<='9')||c=='.'){
				dot=false;
				if(c=='.')dot=true;
				for(i=(c=='.'?cursor+1:cursor);i<s.size();i++){
					c=s[i];
					if(c<'0'||c>'9'){
						if(c=='.'&&!dot)dot=true;
						else break;
					}
				}
				tkns->push_back(new Token(NUMBER,s.substr(cursor,i-cursor),lexer_line_number));
				cursor=i;
			} else {
				cerr<<":"<<lexer_line_number<<":Undefined token at <"<<s[cursor]<<">(cc="<<(int)s[cursor]<<",cur="<<(int)cursor<<")"<<endl;
				exit(1);
			}
			justDidOperator=false;
			break;
		}
	}
	//*str=s.substr(from,cursor-from);
	//trim(*str);
	//cerr<<"return "<<cursor<<" from readFromString"<<endl;
	return cursor;
}
string Statement::getString(void){
	size_t i;
	stringstream *ss=new stringstream;
	string ans;
	for(i=0;i<tkns->size();i++)*ss<<tkns->at(i)->getString()<<",";
	ans=ss->str();
	delete ss;
	ans.erase(ans.size()-1); //remove last comma
	return ans;
}

Program::Program(string s){
	//cerr<<"Parsing <"<<s<<">..."<<endl;
	size_t cursor=0,retval;
	Statement *stmt;
	stmts=new vector<Statement*>;
	while(cursor<s.size()){
		stmt=new Statement;
		retval=stmt->readFromString(s,cursor);
		if(retval==string::npos){
			delete stmt;
			break;
		} else cursor=retval;
		stmts->push_back(stmt);
		//cerr<<"Program: readFromString returned "<<cursor<<", s.size()="<<s.size()<<endl;
	}
	//cerr<<"Finished parsing <"<<s<<">!"<<endl;
}
Program::Program(const Program &other){
	stmts=new vector<Statement*>(*other.stmts);
}
Program::~Program(void){
	delete stmts;
}
string Program::listStatements(void){
	size_t i;
	int t;
	string ss_str;
	stringstream *ss=new stringstream;
	*ss<<stmts->size()<<" statement"<<(stmts->size()==1?"":"s")<<":"<<endl;
	for(i=0;i<stmts->size();i++){
		//cerr<<*ss.str();
		for(t=0;t<prog_list_statements_depth;t++)*ss<<"\t";
		prog_list_statements_depth++;
		*ss<<i<<": "<<stmts->at(i)->getString()<<endl;
		prog_list_statements_depth--;
	}
	for(t=0;t<prog_list_statements_depth-1;t++)*ss<<"\t";
	ss_str=ss->str();
	delete ss;
	return ss_str;
}
void Program::buildAST(ASTnode *astroot){
	size_t i;
	ASTnode *node,*node2;
	Statement *stmt;
	vector<Token*> *tkns;
	for(i=0;i<stmts->size();i++){
		//cerr<<i<<"/"<<stmts->size()<<endl;
		stmt=stmts->at(i);
		tkns=stmt->tkns;

		if(!(tkns->at(tkns->size()-1)->type==SYMBOL&&*tkns->at(tkns->size()-1)->tknstr==";")){
			cerr<<":"<<tkns->at(tkns->size()-1)->linenum<<":Statement does not end in a semicolon...?"<<endl;
			exit(1);
		}

		if(tkns->size()>=1&&tkns->at(0)->type==BLOCK){
			if(tkns->size()>=2&&tkns->at(1)->type==WORD&&((*tkns->at(1)->tknstr)=="while"||(*tkns->at(1)->tknstr)=="until")){
				if(tkns->size()>=4){
					node=new ASTnode(LOOP);
					node2=new ASTnode(LOOP_BLOCK);
					tkns->at(0)->blockprog->buildAST(node2);
					node->attachChild(node2);
					node2=new ASTnode(LOOP_TYPE);
					node2->str=new string(*tkns->at(1)->tknstr);
					node->attachChild(node2);
					node2=new ASTnode(CONDITION);
					Program::buildExpressionAST(node2,tkns,2,tkns->size()-2);
					/*for(j=2;j<tkns->size()-1;j++){
						node3=new ASTnode(EXPR_NUM);
						node3->str=new string(*tkns->at(j)->tknstr);
						node2->attachChild(node3);
					}*/
					node->attachChild(node2);
					astroot->attachChild(node);
				} else {
					cerr<<":"<<tkns->at(tkns->size()-1)->linenum<<":Incomplete statement, too few tokens for a loop"<<endl;
					exit(1);
				}
			} else {
				cerr<<":"<<tkns->at(tkns->size()-1)->linenum<<":Unrecognised statement: loose block"<<endl;
				exit(1);
			}
		} else if(tkns->size()>=4&&tkns->at(tkns->size()-3)->type==SYMBOL&&(*tkns->at(tkns->size()-3)->tknstr=="->"||*tkns->at(tkns->size()-3)->tknstr=="->>")){
			node=new ASTnode(ASSIGN);
			node2=new ASTnode(EXPR);
			Program::buildExpressionAST(node2,tkns,0,tkns->size()-4);
			/*for(j=0;j<tkns->size()-3;j++){
				node3=new ASTnode(EXPR_NUM);
				node3->str=new string(*tkns->at(j)->tknstr);
				node2->attachChild(node3);
			}*/
			node->attachChild(node2);
			node2=new ASTnode(ASSIGN_DEST);
			node2->str=new string(*tkns->at(tkns->size()-2)->tknstr);
			node->attachChild(node2);
			astroot->attachChild(node);
		} else {
			cerr<<":"<<tkns->at(tkns->size()-1)->linenum<<":Unrecognised statement"<<endl;
			exit(1);
		}
	}
}
string Program::compile(ASTnode *astroot){
	//cout<<astroot->printDOT();
	return astroot->compile(0);
}
void Program::buildExpressionAST(ASTnode *root,vector<Token*> *tkns,int startidx,int endidx){
	class Operator{
	public:
		int prec;
		string *str;
		Operator(int _p,string _s){prec=_p;str=new string(_s);}
		Operator(int _p,const char *_s) {prec=_p;str=new string(_s);}
		~Operator(void){delete str;}
	};
	//This function uses Dijkstra's Shunting-yard algorithm.
	//Comments either contain normal comment content or omissions from the real algorithm as copied from Wikipedia ("OM:: ").
	vector<ASTnode*> *rpnstack;
	vector<Operator*> *opstack;
	ASTnode *node,*node2;
	Operator *oper;
	int i,prec/*,termsInRPN*/;
	rpnstack=new vector<ASTnode*>;
	opstack=new vector<Operator*>;
	for(i=startidx;i<=endidx;i++){
		/*cerr<<endl<<"---------"<<endl<<endl<<"rpnstack="<<endl;
		for(ASTnode *nnn:*rpnstack)cerr<<*nnn->str<<": "<<nnn->printDOT();
		cerr<<"opstack="<<endl;
		for(Operator *op:*opstack)cerr<<*op->str<<" ("<<op->prec<<")"<<endl<<endl;*/
		switch(tkns->at(i)->type){
		case NUMBER:
			node=new ASTnode(EXPR_NUM);
			*node->str=*tkns->at(i)->tknstr;
			rpnstack->push_back(node);
			break;
		case STRING:
			node=new ASTnode(EXPR_STR);
			*node->str=*tkns->at(i)->tknstr;
			rpnstack->push_back(node);
			break;
		case WORD:
		case SYMBOL:
			prec=getOperatorPrecedence(*tkns->at(i)->tknstr);
			if(prec==-1){
				if(tkns->at(i)->type==WORD){
					//OM:: If the token is a function token, then push it onto the stack.
					node=new ASTnode(EXPR_WORD);
					*node->str=*tkns->at(i)->tknstr;
					rpnstack->push_back(node);
				} else {
					//OM:: If the token is a function argument separator (e.g., a comma):
					//OM::   Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue. If no left parentheses are encountered, either the separator was misplaced or parentheses were mismatched.
					if(*tkns->at(i)->tknstr=="("){
						oper=new Operator(1e5,"(");
						opstack->push_back(oper);
					} else if(*tkns->at(i)->tknstr==")"){
						while(opstack->size()>0&&*opstack->at(opstack->size()-1)->str!="("){
							node=new ASTnode(EXPR_OPER);
							*node->str=*opstack->at(opstack->size()-1)->str;
							if(rpnstack->size()<2)cerr<<":"<<tkns->at(i)->linenum<<":Invalid expression"<<endl;
							node2=new ASTnode(*rpnstack->at(rpnstack->size()-2));
							node->attachChild(node2);
							node2=new ASTnode(*rpnstack->at(rpnstack->size()-1));
							node->attachChild(node2);
							rpnstack->pop_back();
							rpnstack->pop_back();
							rpnstack->push_back(node);
							opstack->pop_back();
						}
						if(opstack->size()==0){
							cerr<<":"<<tkns->at(i)->linenum<<":Mismatched closing parenthesis"<<endl;
							exit(1);
						}
						opstack->pop_back();
						//OM:: If the token at the top of the stack is a function token, pop it onto the output queue.
					} else {
						cerr<<":"<<tkns->at(i)->linenum<<":Unknown symbol '"<<*tkns->at(i)->tknstr<<"'"<<endl;
						exit(1);
					}
				}
			} else {
				while(opstack->size()>0&&(
					prec>opstack->at(opstack->size()-1)->prec //> instead of < because precedence numbers are reversed here :)
					||(*tkns->at(i)->tknstr!="!"&&*tkns->at(i)->tknstr!="not"&&prec==opstack->at(opstack->size()-1)->prec)
					)){ //`!` and `not` aren't left-associative
					node=new ASTnode(EXPR_OPER);
					*node->str=*opstack->at(opstack->size()-1)->str;
					if(rpnstack->size()<2)cerr<<":"<<tkns->at(i)->linenum<<":Invalid expression"<<endl;
					node2=new ASTnode(*rpnstack->at(rpnstack->size()-2));
					node->attachChild(node2);
					node2=new ASTnode(*rpnstack->at(rpnstack->size()-1));
					node->attachChild(node2);
					rpnstack->pop_back();
					rpnstack->pop_back();
					rpnstack->push_back(node);
					opstack->pop_back();
				}
				oper=new Operator(prec,*tkns->at(i)->tknstr);
				opstack->push_back(oper);
			}
			break;
		case BLOCK:
			cerr<<":"<<tkns->at(i)->linenum<<":Unexpected block in expression"<<endl;
			exit(1);
		default:
			cerr<<":"<<tkns->at(i)->linenum<<":SYSTEM: Unknown tokentype "<<tkns->at(i)->type<<"!"<<endl;
			exit(1);
		}
		/*cerr<<"---------"<<endl<<"rpnstack="<<endl;
		for(ASTnode *nnn:*rpnstack)cerr<<*nnn->str<<": "<<nnn->printDOT();
		cerr<<"opstack="<<endl;
		for(Operator *op:*opstack)cerr<<*op->str<<" ("<<op->prec<<")"<<endl;*/
	}
	//cerr<<"@@@@@@@@@@"<<endl;
	while(opstack->size()>0){
		if(*opstack->at(opstack->size()-1)->str=="("){
			cerr<<":"<<tkns->at(endidx)->linenum<<":Missing closing parenthesis"<<endl;
			exit(1);
		}
		node=new ASTnode(EXPR_OPER);
		*node->str=*opstack->at(opstack->size()-1)->str;
		if(rpnstack->size()<2)cerr<<":"<<tkns->at(i)->linenum<<":Invalid expression"<<endl;
		node2=new ASTnode(*rpnstack->at(rpnstack->size()-2));
		node->attachChild(node2);
		node2=new ASTnode(*rpnstack->at(rpnstack->size()-1));
		node->attachChild(node2);
		rpnstack->pop_back();
		rpnstack->pop_back();
		rpnstack->push_back(node);
		opstack->pop_back();
		/*cerr<<"---------"<<endl<<"rpnstack="<<endl;
		for(ASTnode *nnn:*rpnstack)cerr<<*nnn->str<<": "<<nnn->printDOT();
		cerr<<"opstack="<<endl;
		for(Operator *op:*opstack)cerr<<*op->str<<" ("<<op->prec<<")"<<endl;*/
	}
	if(rpnstack->size()!=1){
		cerr<<":"<<tkns->at(startidx)->linenum<<":Invalid expression (too many tokens, namely "<<rpnstack->size()<<")"<<endl;
		exit(1);
	}
	node=new ASTnode(*rpnstack->at(0));
	root->attachChild(node);
	delete rpnstack;
	delete opstack;
}

class Argflags{
public:
	bool list,nocompile;
	string outfilename;
	string ASToutfilename;
	Argflags(void){
		list=false;
		nocompile=false;
		outfilename="a.out";
		ASToutfilename="";
	}
};

int main(int argc,char **argv){
	int i;
	size_t j;
	bool skipNextArgItem;
	string progstr,line;
	Argflags flags;
	if(argc<2){
		cerr<<"Flow interpreter by Tom Smeding"<<endl;
		cerr<<"Usage: "<<argv[0]<<" [options] <progfile>"<<endl;
		cerr<<"\t-a \x1B[3mfile\x1B[0m\tFile name to output AST to in dot format. Mostly for debugging."<<endl;
		cerr<<"\t-C\tStop after translation to C++."<<endl;
		cerr<<"\t-l\tList the statements after parsing. Mostly for debugging."<<endl;
		cerr<<"\t-o \x1B[3mfile\x1B[0m\tFile name for output file."<<endl;
		return 1;
	}
	if(argc>2){
		skipNextArgItem=false;
		for(i=1;i<argc-1;i++){
			if(skipNextArgItem){
				skipNextArgItem=false;
				continue;
			}
			if(argv[i][0]!='-'){
				cerr<<"Unrecognised command-line parameter \""<<argv[i]<<"\""<<endl;
				return 1;
			}
			for(j=1;j<strlen(argv[i]);j++)switch(argv[i][j]){
			case 'a':
				if(i==argc-2){
					cerr<<"Too few arguments after usage of -a flag."<<endl;
					return 1;
				}
				flags.ASToutfilename=argv[i+1];
				skipNextArgItem=true;
				break;
			case 'C':
				flags.nocompile=true;
				break;
			case 'l':
				flags.list=true;
				break;
			case 'o':
				if(i==argc-2){
					cerr<<"Too few arguments after usage of -o flag."<<endl;
					return 1;
				}
				flags.outfilename=argv[i+1];
				skipNextArgItem=true;
				break;
			default:
				cerr<<"Unrecognised command-line flag \"-"<<argv[i][j]<<"\""<<endl;
				return 1;
			}
		}
	}

	ifstream in(argv[argc-1]);
	if(!in.good()){
		cerr<<"Couldn't open file \""<<argv[argc-1]<<"\""<<endl;
		return 1;
	}
	getline(in,line);
	while(in.good()){
		progstr+=line+"\n";
		getline(in,line);
	}
	progstr+=line;
	in.close();
	trim(progstr);

	Program *prog=new Program(progstr);
	if(flags.list)cerr<<prog->listStatements();

	ASTnode *astroot=new ASTnode(ROOT);
	prog->buildAST(astroot);

	if(flags.ASToutfilename!=""){
		ofstream out(flags.ASToutfilename);
		if(!out.good()){
			cerr<<"Couldn't open file \""<<flags.ASToutfilename<<"\""<<endl;
			return 1;
		}
		out<<astroot->printDOT();
		out.close();
	}

	if(flags.nocompile){
		ofstream out(flags.outfilename);
		if(!out.good()){
			cerr<<"Couldn't open file \""<<flags.outfilename<<"\""<<endl;
			return 1;
		}
		out<<prog->compile(astroot);
		out.close();
	} else {
		char tempfname[16];
		FILE *tempf;
		stringstream cmd;
		strcpy(tempfname,"flow_XXXXXX.cpp");
		tempf=fdopen(mkstemps(tempfname,4),"w");
		fprintf(tempf,"%s",prog->compile(astroot).c_str());
		fclose(tempf);
		cmd<<"g++ -Wall -O2 -xc++ -std=c++11 -o '";
		for(char c : flags.outfilename){
			if(c=='\'')cmd<<"\\'";
			else cmd<<c;
		}
		cmd<<"' "<<tempfname;
		system(cmd.str().c_str());
		unlink(tempfname);
	}

	delete astroot;
	delete prog;
	return 0;
}
