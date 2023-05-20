/**
* @file auto.cpp
* @brief AtCoderのサンプルケース試行を自動化するスクリプト
* @author Kosuke Sugimoto
* @date 2023/4/30
*
* @note コンパイル時は「g++ auto.cpp -lcurl」のようにオプションを付加して実行すること
* @note 実行時には「./a.out "コンテスト名" "問題難易度"」とすること(すべて小文字)
*/

// doxygen

#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<tuple>
#include<cmath>
#include<curl/curl.h>
#include<sys/wait.h>
#include<unistd.h>

using namespace std;

#define R (0)
#define W (1)

/**
 * @brief CURLOPT_WRITEFUNCTIONとして渡す関数
 * @param (ptr) データチャンクから受け取るデータ(ランダムに区切られている)
 * @param (size) 常に1(実際に確認してみても1だった)
 * @param (nmemb) データチャンクから受け取るデータ(ptr)のサイズ
 * @param (stream) CURLOPT_WRITEDATAで指定したバッファへのポインタ
 * @return CURL_OKを返す
 */
size_t write_callback(char* ptr, size_t size, size_t nmemb, string* stream){
    size_t real_nmemb = size*nmemb;
    stream->append(ptr, real_nmemb);
    return real_nmemb;
}

/**
 * @brief 渡された文字列から、指定されたタグで囲まれた部分を抽出する関数
 * @param str 抽出元の文字列
 * @param st_tag 始まりのタグ
 * @param ed_tag 終わりのタグ
 * @return 指定されたタグで囲まれた部分を該当するだけリスト形式にして返す
 */
vector<string> get_splitted_by_tag(string str, string st_tag, string ed_tag){
    int start = 0, end, st_tag_len = st_tag.size();
    vector<string> ret;
    while((start=str.find(st_tag, start))!=string::npos){
        end = str.find(ed_tag, start);
        ret.push_back(str.substr(start+st_tag_len, (end-start-st_tag_len)));
        start += 1;
    }
    return ret;
}

/**
 * @brief 渡された文字列から、サンプルケースを抽出する関数
 * @param str 抽出元の文字列
 * @return keyに何番目のI/Oかを表す文字列を、valueにI/Oの値を持つMapを返す
 */
map<string, string> get_sample_io(string str){
    string key, value;
    map<string, string> mp;
    vector<string> vec, tmp;
    vec = get_splitted_by_tag(str, "<section>", "</section>");
    for (string s: vec){
        tmp = get_splitted_by_tag(s, "<h3>", "</h3>");
        if (tmp[0].find("Sample Input")!=string::npos || tmp[0].find("Sample Output")!=string::npos){
            key = tmp[0];
            tmp = get_splitted_by_tag(s, "<pre>", "</pre>");
            value = tmp[0];
            mp[key] = value;
        }else{
            continue;
        }
    }
    return mp;
}

/**
 * @brief libcurlで渡されたURLからhtmlを抽出～後始末まで一括で行う関数
 * @param url htmlを抽出したいURL
 * @return firstに実行結果(正常終了等)、secondに抽出したhtmlの文字列を持つtupleを返す
 */
tuple<CURLcode, string> curl_flow(string url){
    CURLcode response;
    CURL *curl_handle;
    string ret = "";

    // ハンドラの初期化
    curl_handle = curl_easy_init();

    // nullチェック
    if (curl_handle == nullptr){
        curl_easy_cleanup(curl_handle);
        exit(1);
    }

    // libcurlの設定
    // 接続先URLを設定
    // string型だとエラーを返すため、C言語の文字列表現に変換(NULLで終端)
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    // サーバーのSSL証明書の検証をしないように設定
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, false);
    // レスポンスのコールバック関数設定
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    // 書き込みバッファを指定
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &ret);

    // データを取得
    response = curl_easy_perform(curl_handle);

    // 後片付け
    curl_easy_cleanup(curl_handle);

    return make_tuple(response, ret);
}

/**
 * @brief 渡された文字列のエスケープシーケンスを削除する関数
 * @param str エスケープシーケンスを削除したい文字列
 * @return エスケープシーケンス削除後の文字列を返す
 */
string erase_escape_sequence(string &str){
    int pos = 0;
    string ret="";
    const char CR = '\r';
	const char LF = '\n';

	for (string::const_iterator it=str.begin();it!=str.end();++it) {
		if (*it != CR && *it != LF) {
			ret += *it;
		}
	}

	return ret;
}

/**
 * @brief 渡されたコマンドを引数も渡しつつ実行し、その出力を返す関数
 * @param cmd 実行したいコマンド
 * @param argv 実行したいコマンドに渡したい引数の文字列
 * @return コマンドの実行結果をstring型で返す
 */
string get_cmd_res(string cmd, string argv){
    char buffer[1024] = {};
    int _pipe[2];
    int pid, bytelen;
    FILE* fpout;

    if(pipe(_pipe) < 0){
        exit(EXIT_FAILURE);
    }
    
    pid = fork();

    if(pid < 0){
        // forkに失敗したならばパイプはどちらも閉じる
        close(_pipe[R]); close(_pipe[W]);
        perror("fork");
        exit(1);
    }

    if(pid == 0){
        // 子プロセスでの処理
        // 読み取る必要はないので早々に読み取りパイプは閉じる
        close(_pipe[R]);
        // stdoutを書き込みパイプと繋げたら閉じる
        dup2(_pipe[W], W); close(_pipe[W]);

        // プログラム実行＆出力作業(stdoutへ)
        // プロセスを開く
        fpout = popen((cmd).c_str(), "w");
        // 標準入力を送る
        fputs((argv).c_str(), fpout);
        // プロセスを閉じる
        pclose(fpout);
    }

    // 親プロセスでの処理
    // 書き込む必要はないので書き込みパイプは閉じる
    close(_pipe[W]);
    // パイプから読み込み
    int size = read(_pipe[R], buffer, 1024);
    string s(buffer);

    return s;
}

/**
 * @brief 取得したサンプルケースに対してスクリプトを実行し、正誤を判定、出力する関数
 * @param mp 取得したサンプルケースをI/O共に格納しているMap
 * @return 返り値はない
 */
void run_samplecase(map<string, string> mp){
    int status;
    string compile_cmd = "g++ For_Contest/test.cpp -o For_Contest/test";
    string run_cmd = "For_Contest/test";

    // こちらはコマンドライン引数で問題ないのでsystemを用いる
    status = system(compile_cmd.c_str());
    if (status != 0){
        cout << "Compile Error!!" << endl;
        exit(1);
    }

    // AtCoderの入力は標準入力であるため、systemではなくpopenを利用
    int pos;
    string num, res, output;
    for (auto i : mp){
        pos = i.first.find("Sample Input ");
        if (pos != string::npos){
            // 文字列"Sample Input "は12文字、次の1文字が欲しいため+13
            num = i.first.substr(pos+13, 1);

            res = get_cmd_res(run_cmd, i.second);
            
            output = mp["Sample Output "+num];
            
            // 正解でもエスケープシーケンスの量で不一致なる場合がある
            res = erase_escape_sequence(res);
            output = erase_escape_sequence(output);
            // エスケープシーケンスを消して尚、末尾に空白が入っている場合がある
            if(abs((int)(res.size()-output.size()))==1){
                if(res.back()==' '){
                    res.pop_back();
                }else if(output.back()==' '){
                    output.pop_back();
                }
            }
            
            if(res==output){
                cout << "[" << i.first << "] : OK!" << endl; 
            }else{
                cout << "[" << i.first << "] : NG!" << endl;
                cout << "Your Answer is " << endl << res << endl;
                cout << "Correct Answer is " << endl << output << endl;
                exit(0);
            }
        }else{
            continue;
        }
    }

    cout << "All Sample Case Clear!" << endl;
}

int main(int argc, char* argv[]){
    // 接続先URL
    // 引数で接続先のコンテスト、問題を指定できる
    // 第一引数にコンテスト名(abc298等)、第二引数に問題(a等)を指定
    string url = "";
    if (argc != 1){
        string tmp1 = (string)argv[1], tmp2 = (string)argv[2];
        url = "https://atcoder.jp/contests/" + tmp1 + "/tasks/" + tmp1 + "_" + tmp2;
    }else{
        cout << "Please pass [contest name] as first arg, [difficulty] as second arg." << endl;
        return 1;
    }

    tuple<CURLcode, string> pr = curl_flow(url);

    // 出力
    if(get<0>(pr)==CURLE_OK){
        map<string, string> mp = get_sample_io(get<1>(pr));
        run_samplecase(mp);
    }else{
        cout << "curl error" << get<0>(pr) << endl;
    }
    return 0;
}