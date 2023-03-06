
#include <iostream>
#include<regex>
#include<string>
using namespace std;
int main(){
    regex reg("^(?![0-9]+$)(?![a-zA-Z]+$)[0-9A-Za-z]{8,16}$");//密码包含数字和字母 8-16位
    string str;
    cin>>str;
    cout << regex_match(str, reg);
    return 0;
}
