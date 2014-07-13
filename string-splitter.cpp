/*
  The MIT License (MIT)

  Copyright(C) 2014 Naoya Murakami <naoya@createfield.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <vector>

#include <unicode/normlzr.h>
#include <mecab.h>
#include <re2/re2.h>
#include <wn.h>

#include <google/gflags.h>

DEFINE_string(input, "", "Input file path");
DEFINE_string(pre_filter,
              "(<[^>]*>)|\\\\n|\\\\r|\\\\t|([\\\\,.;:&^/\\-\\#'\"\\(\\)\\[\\]{}])",
              "Regular expresion pattern to input string");
DEFINE_bool(no_normalize, false, "Don't use NFKC normalize");
DEFINE_bool(no_tokenize, false, "Don't use tokenize");
DEFINE_string(mecab_dic, "", "Specifying mecab dictionary path");
DEFINE_bool(use_baseform, false, "Use baseform by MeCab dictionary");
DEFINE_string(token_filter, "", "Regular expresion pattern to token string");
DEFINE_bool(use_wordnet, false, "Use baseform by WordNet");
DEFINE_bool(cut_prolong, false, "Cut prolong if token have over 3 characters");
DEFINE_bool(h, false, "Help message");

#define MECAB_LIMIT 900000

using namespace std;

string
normalize(const string str)
{
  icu::UnicodeString src;
  icu::UnicodeString dest;
  src = str.c_str();
  UErrorCode status;
  status = U_ZERO_ERROR;
  Normalizer::normalize(src, UNORM_NFKC, 0, dest, status);
  if(U_FAILURE(status)) {
    throw runtime_error("Unicode normalization failed");
  }
  string result;
  dest.toUTF8String(result);
  transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

static char *
right_trim(char *s, char del) {
  int i;
  int count = 0;
  if ( s == NULL ) {
    return NULL;
  }
  i = strlen(s);
  while ( --i >= 0 && s[i] == del ) count++;
  if (i == 0) {
    s[i] = '\0';
  } else {
    s[i+1] = '\0';
  }
  return s;
}

/* utf8_fix_pos
   utf8文字列じゃなければ、3バイト以内にutf8があるか判定する
*/
unsigned int
utf8_fix_pos(const string str, unsigned int start, unsigned int end)
{
  unsigned int pos;
  unsigned char lead;
  int char_size;

  for (pos = end; pos > start && pos > end - 3; pos--) {
    lead = str[pos];
    if (lead < 0x80) {
      char_size = 1;
    } else if (lead < 0xE0) {
      char_size = 2;
    } else if (lead < 0xF0) {
      char_size = 3;
    } else {
      char_size = 4;
    }
    if(char_size == 1 || char_size == 3){
       return pos;
    }
  }
  return end;
}

/* range_rfind_punct
  utf8文字列の範囲内を末尾から区切り文字、半角スペース、改行コード、
  日本語区切り文字を探す。
  見つかればその位置。見つからなければ、末尾の位置。
*/

int
range_rfind_punct(const string str, unsigned int start, unsigned int end)
{
  unsigned int pos;
  unsigned char lead;
  int char_size;

  for (pos = end; pos > start; pos -= char_size) {
    lead = str[pos];
    if (lead < 0x80) {
      char_size = 1;
    } else if (lead < 0xE0) {
      char_size = 2;
    } else if (lead < 0xF0) {
      char_size = 3;
    } else {
      char_size = 4;
    }
    
    if ((lead >= 0x20 && lead < 0x2f) ||
        (lead >= 0x3a && lead <= 0x40) ||
        (lead >= 0x5b && lead <= 0x60) ||
        (lead >= 0x7b && lead <= 0x7d) ||
        (lead >= 0x0a && lead <= 0x0d) ||
        str.substr(pos, char_size) == "。" ||
        str.substr(pos, char_size) == "、" ||
        str.substr(pos, char_size) == "「" ||
        str.substr(pos, char_size) == "」" ||
        str.substr(pos, char_size) == "・" 
       ) {
       return pos;
    }
  }

  lead = str[end];
  if (lead < 0x80) {
    char_size = 1;
  } else if (lead < 0xE0) {
    char_size = 2;
  } else if (lead < 0xF0) {
    char_size = 3;
  } else {
    char_size = 4;
  }
  if(char_size == 2 || char_size == 4){
    pos = utf8_fix_pos(str, start, end);
    return pos;
  }
  return end;
}


string
tokenize(const string str, const string mecab_dic, bool baseform)
{
  mecab_t *mecab = NULL;
  string mecab_option;
  
  if(baseform){
    mecab_option = "-F%f[6]\\s -E\\n";
  } else {
    mecab_option = "-Owakati";
  }
  if(mecab_dic.length() > 0){
    mecab_option += " -d " + mecab_dic;
  }

  mecab = mecab_new2(mecab_option.c_str());

  string result;
  if(str.length() < MECAB_LIMIT) {
    result = mecab_sparse_tostr2(mecab, str.c_str(), str.length());
    result = right_trim((char *)result.c_str(), '\n');
  } else {
    unsigned int punct_pos = 0;
    unsigned int start = 0;
    unsigned int end = MECAB_LIMIT;
    unsigned int split_len;
    while(end < str.length()){
      punct_pos = range_rfind_punct(str, start, end);
      split_len = punct_pos - start;
      result += mecab_sparse_tostr2(mecab, str.c_str() + start, split_len);
      result = right_trim((char *)result.c_str(), '\n');
      start = punct_pos;
      end = start + MECAB_LIMIT;
    }
    split_len = str.length() - start;
    result += mecab_sparse_tostr2(mecab, str.c_str() + start, split_len);
  }
  mecab_destroy(mecab);
  return result;
}

string
string_filter(string str, const char *pattern)
{
  if(strcmp(pattern, "") != 0){
    re2::RE2::GlobalReplace(&str, pattern, "");
  }
  return str;
}

vector<string>
split(const string &str, char delim){
  vector<string> res;
  size_t current = 0, found;
  while((found = str.find_first_of(delim, current)) != string::npos){
    res.push_back(string(str, current, found - current));
    current = found + 1;
  }
  res.push_back(string(str, current, str.size() - current));
  return res;
}

string
token_filter(const string str, const char *pattern, bool wordnet, bool prolong)
{
  vector<string> result_array;
  result_array = split(str, ' ');
  string result;
  for(unsigned int i = 0; i < result_array.size(); ++i )
  {
    char *buf;
    buf = (char *)result_array[i].c_str();
    if(wordnet){
      int pos;
      char *morph;
      if (wninit() == -1){
        return str;
      }
      //pos 1:NOUN 2:VERB 3:ADJECTIVE 4:ADVERB 5:ADJECTIVE_SATELITE
      for(pos = 5; pos >= 1; pos--){
        morph = morphstr(buf, pos);
        if(morph){
          buf = morph; 
          break;
        }
      }
    }
    bool ret = false;
    string string_buf = buf;

    if(prolong) {
      if(string_buf.length() >= 12 ){
        string tail_buf = string_buf.substr(string_buf.length()-3, 3);
        if(tail_buf == "ー" || tail_buf == "ｰ"){
          string_buf = string_buf.substr(0, string_buf.length()-3);
        }
      }
    }

    if(strcmp(pattern, "") != 0){
      ret = re2::RE2::GlobalReplace(&string_buf, pattern, "");
    }

    result += string_buf;
    if(i < result_array.size() && ret == false){
      result += " "; 
    }
  }
  return result;
}

int
string_splitter(string str)
{
  str = string_filter(str, FLAGS_pre_filter.c_str());
  if (!FLAGS_no_normalize) {
    str = normalize(str);
  }
  if (!FLAGS_no_tokenize) {
    str = tokenize(str, FLAGS_mecab_dic, FLAGS_use_baseform);
  }
  if (FLAGS_token_filter != "" || FLAGS_use_wordnet || FLAGS_cut_prolong){
    str = token_filter(str, FLAGS_token_filter.c_str(), FLAGS_use_wordnet, FLAGS_cut_prolong);
  }
  cout << str.c_str() << endl;
  return 0;
}

int
main(int argc,char **argv)
{
  google::ParseCommandLineFlags(&argc, &argv, true);

  if(FLAGS_h == true){
    printf("  --iuput : Input file path\n");
    printf("  --pre_filter : Regular expresion pattern to input string\n");
    printf("  --no_normalize : Don't use NFKC normalize\n");
    printf("  --no_tokenize : Don't use tokenize\n");
    printf("  --mecab_dic : Specifying mecab dictionary path\n");
    printf("  --use_baseform : Convert baseform by MeCab dictionary\n");
    printf("  --token_filter : Regular expresion pattern to token string\n");
    printf("  --use_wordndet : Convert baseform by WordNet\n");
    printf("  --cut_prolong : Cut prolong if token have over 3 characters\n");
    exit(0);
  }

  string str;
  if(FLAGS_input == ""){
    while(getline(cin, str)){
      if(str == "EOS"){
        break;
      }
      string_splitter(str);
    }
  } else {
    ifstream ifs(FLAGS_input.c_str());
    if(ifs.fail()) {
      cerr << "File do not exist.\n";
      exit(0);
    }
    while(getline(ifs, str)){
      string_splitter(str);
    }
  }

  return 0;
}
