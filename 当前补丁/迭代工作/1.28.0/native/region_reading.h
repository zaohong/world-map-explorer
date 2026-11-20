#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace regionReading {
inline std::wstring trim(std::wstring s){while(!s.empty()&&iswspace(s.front()))s.erase(s.begin());while(!s.empty()&&iswspace(s.back()))s.pop_back();return s;}
inline bool known(const std::wstring& text){
 if(text.empty())return false;
 for(const auto* empty:{L"未收录",L"尚未",L"暂无",L"未匹配",L"未获得",L"没有命中",L"尚无",L"档案未加载",L"未安装",L"尚未发现"})if(text.find(empty)!=std::wstring::npos)return false;
 return true;
}
inline std::vector<std::wstring> split(std::wstring s){
 for(size_t p=0;(p=s.find(L"\\n",p))!=std::wstring::npos;)s.replace(p,2,L"\n");
 std::vector<std::wstring> out;size_t begin=0;for(size_t i=0;i<=s.size();++i)if(i==s.size()||s[i]==L'；'||s[i]==L';'||s[i]==L'\n'){auto part=trim(s.substr(begin,i-begin));if(!part.empty())out.push_back(part);begin=i+1;}return out;
}
inline bool latin(const std::wstring& s){for(auto c:s)if((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z'))return true;return false;}
inline std::wstring placeName(const std::wstring& input,bool english){
 auto s=trim(input);size_t p=s.find(L'（');if(p==std::wstring::npos)return s;
 size_t q=s.find(L'）',p+1);if(q==std::wstring::npos)return trim(s.substr(0,p));
 auto alternative=s.substr(p+1,q-p-1);
 if(english&&latin(alternative)&&alternative.find(L"人口")==std::wstring::npos&&alternative.find(L"样点")==std::wstring::npos)return alternative;
 return trim(s.substr(0,p));
}
inline std::wstring names(const std::wstring& text,size_t maximum,bool english){
 if(!known(text))return L"";std::vector<std::wstring> used;std::wstring out;
 for(const auto& item:split(text)){auto name=placeName(item,english);if(name.empty()||name==L"亞洲"||name==L"亚洲"||name==L"Asia"||name==L"Europe"||name==L"非洲"||name==L"Africa"||std::find(used.begin(),used.end(),name)!=used.end())continue;used.push_back(name);if(!out.empty())out+=english?L" · ":L"、";out+=name;if(used.size()>=maximum)break;}
 return out;
}
inline std::wstring heritageNames(const std::wstring& source,size_t maximum,bool english){
 std::wstring text=source,out;for(size_t p=0;(p=text.find(L"\\n",p))!=std::wstring::npos;)text.replace(p,2,L"\n");
 size_t count=0;for(const auto& line:split(text)){if(line.empty()||line[0]!=L'#')continue;size_t space=line.find(L' ');if(space==std::wstring::npos)continue;auto title=line.substr(space+1);size_t metadata=title.find(L"（文化遗产");if(metadata==std::wstring::npos)metadata=title.find(L"（自然遗产");if(metadata==std::wstring::npos)metadata=title.find(L"（混合遗产");if(metadata!=std::wstring::npos)title.resize(metadata);
  if(english){size_t p=0;bool found=false;while((p=title.find(L'（',p))!=std::wstring::npos){size_t q=title.find(L'）',p+1);if(q==std::wstring::npos)break;auto alternate=title.substr(p+1,q-p-1);if(latin(alternate)){title=alternate;found=true;break;}p=q+1;}if(!found)title=placeName(title,true);}
  else{size_t p=0;while((p=title.find(L'（',p))!=std::wstring::npos){size_t q=title.find(L'）',p+1);if(q==std::wstring::npos)break;if(latin(title.substr(p+1,q-p-1))){title.resize(p);break;}p=q+1;}}
  if(!out.empty())out+=L"\n";out+=L"• "+trim(title);if(++count>=maximum)break;
 }return out;
}
inline std::wstring compactPopulation(double value,bool english){
 if(!std::isfinite(value)||value<0)return english?L"No estimate":L"暂无估值";
 std::wostringstream out;double divisor=1;const wchar_t* unit=L"";
 if(english){if(value>=1e9){divisor=1e9;unit=L"B";}else if(value>=1e6){divisor=1e6;unit=L"M";}else if(value>=1e3){divisor=1e3;unit=L"K";}}
 else{if(value>=1e8){divisor=1e8;unit=L"亿";}else if(value>=1e4){divisor=1e4;unit=L"万";}}
 out<<(english?L"~":L"约 ")<<std::fixed<<std::setprecision(divisor>1?1:0)<<value/divisor<<unit;return out.str();
}
inline std::wstring number(double value,int precision=0){std::wostringstream out;out<<std::fixed<<std::setprecision(precision)<<value;return out.str();}
}
