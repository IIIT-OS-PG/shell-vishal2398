#include<stdio.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<iostream>
#include<string.h>
#include<deque>
#include<typeinfo>
#include<fcntl.h>
#include<bits/stdc++.h>
#include<map>
using namespace std;
/*
int savstdin = dup2(STDIN_FILENO,savstdin);
int savstdout = dup2(STDOUT_FILENO,savstdout);
*/
int childstatus;
map<string,string> al;
string getext(deque<string> &cmd)
{
	string ext="";
	if(cmd[0] == "open")
	{
		string l = cmd[1];
		//cout<<"cmd[1]"<<l<<endl;
		int i=0;
		while(i<l.length())	//to be filled later after creating map
		{
			if(l[i] == '.')
			{
				i++;
				while(i<l.length() && l[i] != ' ')
				{
					ext = ext + l[i];
					i++;
				}
			}
			i++;
		}
	}
	return ext;
}
pair<string,string> getkeyval(string cmd)
{
	string key="",value="";
    int i;
    for(i=0;cmd[i] != '=';i++)
    {
        key = key + cmd[i];
    }
    i++;
    while(i<cmd.length())
    {
        value = value+cmd[i];
        i++;
    }
	//cout<<"in getkeyval "<<"key "<<key<<"val "<<value<<endl;
	pair<string,string> ret;
	ret.first = key;
	ret.second = value;
	return ret;
}
void handlespace(string cmd,deque<string> &dq)
{
	//cout<<"in handle space"<<endl;
	//cout<<cmd<<endl;
	string tok="";
    int i=0;
    while(cmd[i] != ' ' && i<cmd.size())
    {
        tok = tok + cmd[i];
        i++;
    }
    i++;
    dq[0] = tok;
    tok="";
    auto it = dq.begin()+1;
    while(i<cmd.length())
    {
        if(cmd[i] == ' ')
        {
            dq.insert(it,tok);
            tok="";
            it++;
        }
        else
        {
            tok = tok + cmd[i];
        }
        i++;
    }
    if(tok != "")
    {
        dq.insert(it,tok);
    }
    //cout<<"printing dq"<<endl;  
    /*for(auto x : dq)
    {
        cout<<x<<" ";
    }*/

}
void storeBashRC(map<string,string> *mp)
{
	FILE *fp;
	fp = fopen("BashRC.txt","r");
    if(fp == NULL)
    {
      //   cout<<"got this far ";
        cout<<"error"<<endl;
        exit(2);
    } 
   char l[100];
    while(fgets(l,100,fp) != NULL)
    {
        string key="",value="";
        for(int i=0;l[i]!='\0' && l[i]!='\n';i++)
        {
            if(l[i] == '=')
            {
                i++;
                while(l[i]!='\0' && l[i]!='\n')
                {
                    value = value+l[i];
                    i++;
                }
                i--;
            }
            else
            {
                key = key+l[i];
            }
        }
        (*mp)[key] = value;
    }
}
void redirout(char *t[],string outfile,bool *pout,bool *doubleredirect)
{
	char temp[100];
	strcpy(temp,outfile.c_str());
	int fdr;
	//close(STDOUT_FILENO);
	if(*doubleredirect == false)
	{
		 fdr = open(temp,O_CREAT | O_WRONLY | O_TRUNC,00644);
	}
	else
	{
		fdr = open(temp,O_CREAT | O_WRONLY | O_APPEND,00644);
	}
	int pid;
	pid = fork();
	if(fdr<0)
	{
		cout<<"error";
		exit(2);
	}
	if(pid < 0)
	{
		perror("error in opening file at redirout ");
	}
	else if(pid == 0)
	{
		dup2(fdr,STDOUT_FILENO);
		close(fdr);
		execvp(t[0],t);
	}
	else
	{
		int childstat;
		if(waitpid(pid,&childstat,0) <0)
		{
			perror("error in child exec in redirin");
		}
		childstatus = childstat;
		*pout = false;
		*doubleredirect = false;
		close(fdr);
	}
}
void redirin(char *t[],string infile,bool *pin)
{
	//cout<<"in redir in "<<endl;
	char temp[100];
	strcpy(temp,infile.c_str());
	int fdr = open(temp,O_RDONLY);
	if(fdr<0)
	{
		cout<<"error";
		exit(2);
	}
	int pid;
	pid = fork();
	if(pid<0)
	{
		perror("error in forking");
	}
	else if(pid == 0)
	{
		dup2(fdr,STDIN_FILENO);
		close(fdr);	
		execvp(t[0],t);
	
	}
	else
	{
		close(fdr);
		int childstat;
		if(waitpid(pid,&childstat,0) <0)
		{
			perror("error in child exec in redirin");
		}
		childstatus = childstat;
		*pin = false;
	}
}
void bothredir(char *t[],string infile,bool *pin,string outfile,bool *pout)
{
//	cout<<"in bothredir"<<endl;
	int fdr,fdw;
	*pin = false;
	*pout = false;
	fdr = open(infile.c_str(),O_RDONLY);
	fdw = open(outfile.c_str(),O_CREAT | O_WRONLY | O_TRUNC,00644);
	if(fdr <0 || fdw <0)
	{
		cout<<"error in bothredir opening files"<<endl;
	}
	int pid = 0;
	pid = fork();
	if(pid == 0)
	{
		dup2(fdr,STDIN_FILENO);
		dup2(fdw,STDOUT_FILENO);
		close(fdr);
		close(fdw);
		execvp(t[0],t);
	}
	else if(pid<0)
	{
		perror("error in forking at bothredir");
	}
	else
	{
		close(fdr);
		close(fdw);
		int childstat;
		if(waitpid(pid,&childstat,0) <0)
		{
			perror("error in child exec in bothredir");
		}
		childstatus = childstat;
		*pin = false;
		*pout = false;
	}
}
void executepipe(int numOfPipes,deque<string> &cmd,bool *lastpipe,bool *in,string infile,bool*out,string outfile,map<string,string> &mp)
{
//	cout<<"in execute pipe"<<endl;
	int i=0;
	
/*	for(auto x : cmd)
	{
		cout<<i<<" "<<x<<endl;
		i++;
	}*/
	auto it = al.find(cmd[0]);
			if(it != al.end())
			{
				string str = it->second;
				//cout<<"real "<<str<<endl;
				handlespace(str,cmd);
			}
	int pid;
	int fdr = open("rdFiLe.txt",O_CREAT | O_RDWR ,00644);
	int fdw = open("wrFiLe.txt",O_CREAT | O_RDWR ,00644);
	int fd1r;
	int fd1w;
	char *t[cmd.size()+1];
	for(int i=0;i<cmd.size();i++)
	{ 
		char *tem = (char*)malloc(sizeof(cmd[i].size() + 1));
		strcpy(tem,cmd[i].c_str());
		t[i] = tem;
		//	cout<<"t[i] "<<t[i]<<'1'<<endl;
	}
	t[cmd.size()] = {NULL};
	if(cmd[0] == "open")
	{
		string ext="";
		ext = getext(cmd);
		char *tem = (char*)malloc(sizeof(mp[ext].size()+1));
		strcpy(tem,mp[ext].c_str());
		t[0] = tem;
	}
	if((pid = fork())  < 0)
	{
		perror("error : ");
	}
	else if(pid == 0)
	{
		if(*lastpipe == false)
		{
			if(numOfPipes == 1)
			{
				if(*in == true)
				{
					*in =false;
					fd1r = open(infile.c_str(),O_RDONLY);
					dup2(fd1r,STDIN_FILENO);
					close(fd1r);
				}
				close(fdr);
				dup2(fdw,STDOUT_FILENO);
				close(fdw);
				execvp(t[0],t);
			}
			
			else if(numOfPipes%2 == 0)
			{
				dup2(fdw,STDIN_FILENO);
				close(fdw);
				dup2(fdr,STDOUT_FILENO);
				close(fdr);
				execvp(t[0],t);
			}
			else if(numOfPipes%2 == 1)
			{
				dup2(fdr,STDIN_FILENO);
				close(fdr);
				dup2(fdw,STDOUT_FILENO);
				close(fdw);
				execvp(t[0],t);
			}
			
		}
		else
		{
			if(numOfPipes%2 == 0)
			{
	//			cout<<"out "<<*out<<endl;
	//			cout<<"outfile"<<outfile<<"1"<<endl;
				if(*out == true)
				{
					*out = false;
					fd1w = open(outfile.c_str(),O_CREAT | O_WRONLY | O_TRUNC,00644);
					dup2(fd1w,STDOUT_FILENO);
					close(fd1w);
				}
				dup2(fdr,STDIN_FILENO);
				close(fdw);
				close(fdr);
				*lastpipe = false;
				execvp(t[0],t);
			}
			else
			{
				
				if(*out == true)
				{
					*out = false;
					fd1w = open(outfile.c_str(),O_CREAT | O_WRONLY | O_TRUNC,00644);
					dup2(fd1w,STDOUT_FILENO);
					close(fd1w);
				}	
				dup2(fdw,STDIN_FILENO);
				close(fdw);
				close(fdr);
				*lastpipe = false;
				execvp(t[0],t);
			}
			
		}
		
	}
	else
	{
		int childstat;
		if(waitpid(pid,&childstat,0) <0)
		{
			perror("error in child exec ");
		}
		childstatus = childstat;
	}
	for(int i=0;i<cmd.size()+1;i++)
	{
		free(t[i]);
	}
	//dup2(savstdin,STDIN_FILENO);
	//dup2(savstdout,STDOUT_FILENO);
	close(fdr);
	close(fdw);
}
int main(void)
{
	int pid;
	int childstat;
	cout<<endl;
	while(1)
	{
			cout<<"acer@acer-Nitro-AN515-51:~$";
			/*
			char *arg[] ={" ","-al",NULL};
			execvp("ls",arg) ;
			/*err_ret("couldn't execute");
			exit(127);*/

			string line,tok="";
			string outfile="";
			string infile="";
			deque<string>cmd;
			getline(cin,line);
			int numOfPipes =0;
			vector<int>pipepos;
			bool lastpipe = false;
			bool out=false,in=false;
			bool doubleredirect = false;
			bool opencmd = false;
			bool alias= false;
			map<string,string> mp;
			storeBashRC(&mp);
	/*		for(auto x: mp)
			{
				cout<<x.first<<" "<<x.second<<endl;
			}*/
			//cout<<mp["$PS1"]<<"$"<<endl;
			
			for(int i=0;i<line.length();i++)
			{
				if(line[i] == '"')
				{
					i++;
					while(line[i] != '"' && i<line.length())
					{
						tok += line[i];
						i++;
					}
					cmd.push_back(tok);
					tok = "";
				}
				else if(line[i] == ' ')
				{
					if(tok!="")
					{
					cmd.push_back(tok);
					}
					//cmd.push_back(" ");
					//cout<<tok<<endl;
					tok="";
				}
				else if(line[i] == '>')
				{
					out = true;
					i++;
					if(line[i] == '>')
					{
						doubleredirect = true;
						i++;
					}
					while(line[i]== ' ' && i<line.length()){i++;}
					if(tok !="")
					{
						cmd.push_back(tok);
						tok="";
					}
					while(line[i] != ' ' && i<line.length() && line[i] != '|')
					{
						outfile = outfile+line[i];
						i++;
					}
					if(line[i] == '|')
					{
						i--;
					}
		//			cout<<outfile<<endl;
				}
				else if(line[i] == '<')
				{
					i++;
					while(line[i]== ' ' && i<line.length()){i++;}
					in = true;
					//while(line[i] == ' ' && i<line.length()){i++;}
					if(tok !="")
					{
						cmd.push_back(tok);
						tok="";
					}
					while(i<line.length() && line[i] != '|' && line[i]!= ' ')
					{
						infile = infile+line[i];
						i++;
					}
		//			cout<<"infile : "<<infile<<"1";
					if(line[i] == '|')
					{
						i--;
						continue;
					}	
				}
				else if(line[i] == '|')
				{
		//			cout<<"tok"<<tok<<endl;
					numOfPipes++;
					executepipe(numOfPipes,cmd,&lastpipe,&in,infile,&out,outfile,mp);
					cmd.clear();
					tok="";
				}
				else
				{
					tok += line[i];
				}	
			}
			if(tok != "")
			{

				cmd.push_back(tok);
				tok="";
			}	
		//	cout<<"num pipes : "<<numOfPipes<<endl;
			int iter=0;
		/*	for(auto x : cmd)
			{
				cout<<iter++;
				cout<<x<<endl;
			}*/
		//	cout<<"cmd[1]"<<cmd[1]<<endl;
			if(cmd[0] == "alias" && cmd.size() >1)
			{
				alias=true;	
				pair<string,string> kvp;
				kvp = getkeyval(cmd[1]);
				al[kvp.first] = kvp.second; 
			}
			if(cmd[0] == "cd")
			{
				chdir(cmd[1].c_str());
				continue;
			}
			if(cmd[0] == "$?")
			{
				char str[100];
				getcwd(str,100);
				cout<<"current working directory : "<<str<<endl;
				continue;
			}
			if(cmd[0] == "$$")
			{
				cout<<"process id : "<<getppid()<<endl;
				continue;
			}
			if(cmd[0] == "alias" && cmd.size()==1)
			{
				if(al.size() > 0)
				{
					cout<<"following are the aliases set : "<<endl;
					for(auto x: al)
					{
						cout<<x.first<<"="<<x.second<<endl;
					}
				}
				else
				{
					cout<<"no alias set"<<endl;
				}
				
			}
			auto it = al.find(cmd[0]);
			if(it != al.end())
			{
				string str = it->second;
				//cout<<"real "<<str<<endl;
				handlespace(str,cmd);
			}
			//cout<<endl<<"outfile "<<outfile<<endl;
			//cout<<"infile "<<infile<<endl;
			//cout<<"out"<<out<<endl;
			//cout<<"in"<<in<<endl;
			//cout<<"opencmd"<<opencmd<<endl;*/
			char *t[cmd.size()+1];
			for(int i=0;i<cmd.size();i++)
			{
				char *tem = (char*)malloc(sizeof(cmd[i].size() + 1));
				strcpy(tem,cmd[i].c_str());
				t[i] = tem;
			}
			t[cmd.size()] = {NULL}; 
			//cmd = ["ls"," ","-al",NULL]
			//cout<<"executing  : " <<endl;
		
			if(cmd[0] == "open")
			{
				string ext="";
				ext = getext(cmd);
				char *tem = (char*)malloc(sizeof(mp[ext].size()+1));
				strcpy(tem,mp[ext].c_str());
				t[0] = tem;
			}
			if(out == false && in == false && numOfPipes==0)
			{
				int pi;
				pi= fork();
				if(pi<0)
				{
					perror("error during normal execution");
				}
				if(pi == 0)
				{
					execvp(t[0],t);
				}
				else
				{
					int chidstat;
					if(waitpid(pi,&childstat,0) < 0)
					{
						perror("error in waiting for normal execution");
					}
					childstatus = childstat;
				}
			}
			else if(numOfPipes > 0)
			{
			//	cout<<"in last pipe";
				lastpipe = true;
				executepipe(numOfPipes,cmd,&lastpipe,&in,infile,&out,outfile,mp);
			}
			else if(out == true && in == false)
			{
			//	cout<<"in redir out : "<<endl;
				redirout(t,outfile,&out,&doubleredirect);
			}
			else if(out == false && in == true)
			{
			//	cout<<"in redirin"<<endl;
				redirin(t,infile,&in);
			}
			else if(out == true && in == true)
			{
				bothredir(t,infile,&in,outfile,&out);	
			}
			
			for(int i=0;i<cmd.size()+1;i++)
			{
				free(t[i]);
			}
			cmd.clear();
			remove("rdFiLe.txt");
			remove("wrFiLe.txt");
		
	}
	return 0;
}
