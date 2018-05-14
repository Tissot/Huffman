#include <iostream>
#include <chrono>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <string>
using namespace std;

#define peseudo_eof 256

ifstream ifs;
ofstream ofs;

struct bin_tree_node {
	int ascii;
	int freq;
	struct bin_tree_node *parent;
	struct bin_tree_node *left_child;
	struct bin_tree_node *right_child;

	bin_tree_node() {}
	bin_tree_node(int freq) : ascii(-1), freq(freq), parent(NULL), left_child(NULL), right_child(NULL) {}
	bin_tree_node(int ascii, int freq) : ascii(ascii), freq(freq), parent(NULL), left_child(NULL), right_child(NULL) {}
	~bin_tree_node() {
		parent = NULL;
		left_child = NULL;
		right_child = NULL;
	}
};

unordered_map<int, int> counting_character(string file_name) {
	unordered_map<int, int> chars_freq;

	ifs.open(file_name);

	if (!ifs.is_open()) {
		cout << "文件打开失败！" << endl;
	} else {
		cout << "压缩中......" << endl << endl;

		char buffer;

		while ((buffer = ifs.get()) != EOF) {
			if (chars_freq.find(buffer) != chars_freq.end()) {
				++chars_freq[buffer];
			} else {
				chars_freq[buffer] = 1;
			}
		}

		if (chars_freq.empty()) {
			cout << "文件内容为空！" << endl;
		}

		ifs.close();

		chars_freq[peseudo_eof] = 1;
	}

	return chars_freq;
}

bin_tree_node *building_huffman_by_freq(unordered_map<int, int> chars_freq) {
	auto cmp = [](const bin_tree_node *x, const bin_tree_node *y) {
		return x->freq > y->freq;
	};
	priority_queue<bin_tree_node *, vector<bin_tree_node *>, decltype(cmp)> pq(cmp);

	for (const auto &iterator : chars_freq) {
		pq.push(new bin_tree_node(iterator.first, iterator.second));
	}

	bin_tree_node *parent = NULL;

	while (pq.size() > 1) {
		parent = new bin_tree_node(0);

		parent->left_child = pq.top();
		parent->left_child->parent = parent;
		parent->freq += pq.top()->freq;
		pq.pop();

		parent->right_child = pq.top();
		parent->right_child->parent = parent;
		parent->freq += pq.top()->freq;
		pq.pop();

		pq.push(parent);
	}

	return parent;
}

void huffman_coding(bin_tree_node *parent, unordered_map<int, string> &chars_code, string current_code) {
	if (parent->left_child != NULL && parent->right_child != NULL) {
		huffman_coding(parent->left_child, chars_code, current_code + '0');
		huffman_coding(parent->right_child, chars_code, current_code + '1');
	} else if (parent->left_child == NULL && parent->right_child == NULL) {
		chars_code[parent->ascii] = current_code;
	}
}

void compress_char(string huffman_code, unsigned char &byte, int &bits_counter) {
	for (char bit : huffman_code) {
		byte = (byte << 1) + (bit - '0');
		++bits_counter;

		if (bits_counter == 8) {
			ofs << byte;
			byte = 0;
			bits_counter = 0;
		}
	}
}

void write_into_compressed_file(unordered_map<int, string> chars_code, string source_file_name) {
	int pos = source_file_name.rfind('.');
	string target_file_name = source_file_name.substr(0, pos) + ".bin";
	char buffer;
	string huffman_code;
	unsigned char byte = 0;
	int bits_counter = 0;

	ofs.open(target_file_name);

	for (const auto &iterator : chars_code) {
		ofs << iterator.first << " " << iterator.second << endl;
	}
	ofs << ".data" << endl;

	ifs.open(source_file_name);

	while ((buffer = ifs.get()) != EOF) {
		compress_char(chars_code.find(buffer)->second, byte, bits_counter);
	}
	compress_char(chars_code.find(peseudo_eof)->second, byte, bits_counter);

	while (bits_counter != 0) {
		byte <<= 1;
		++bits_counter;

		if (bits_counter == 8) {
			break;
		}
	}

	ofs << byte;

	ifs.close();
	ofs.close();
}

void compress_file(string file_name) {
	unordered_map<int, int> chars_freq = counting_character(file_name);

	if (!chars_freq.empty()) {
		cout << "字符统计结果：" << endl;
		for (const auto &iterator : chars_freq) {
			cout << iterator.first << "-" << iterator.second << endl;
		}
		cout << endl;

		bin_tree_node *root = building_huffman_by_freq(chars_freq);

		if (root == NULL) {
			cout << "Huffman 建树失败!" << endl;
		} else {
			unordered_map<int, string> chars_code;

			huffman_coding(root, chars_code, "");

			delete root;

			cout << "字符编码结果：" << endl;
			for (auto &iterator : chars_code) {
				cout << iterator.first << "-" << iterator.second << endl;
			}
			cout << endl;

			write_into_compressed_file(chars_code, file_name);
		}
	}
}

unordered_map<string, int> get_codes_char () {
	string head_line;
	int pos;
	int ascii;
	string huffman_code;
	unordered_map<string, int> codes_char;

	getline(ifs, head_line);

	while (head_line != ".data") {
		pos = head_line.find(' ');
		ascii = 0;

		for (char bit :  head_line.substr(0, pos)) {
			ascii = ascii * 10 + (bit - '0');
		}

		codes_char[head_line.substr(pos + 1, head_line.size())] = ascii;
		getline(ifs, head_line);
	}

	cout << "获取字符编码：" << endl;
	for (auto &iterator : codes_char) {
		cout << iterator.first << "-" << iterator.second << endl;
	}
	cout << endl;

	return codes_char;
}

void write_into_uncompressed_file (unordered_map<string, int> codes_char, string source_file_name) {
	int pos = source_file_name.rfind('.');
	string target_file_name = source_file_name.substr(0, pos) + ".txt";
	bool peseudo_eof_found = false;

	ofs.open(target_file_name);

	unsigned char buffer;
	string current_code;

	while (peseudo_eof_found == false) {
		buffer = ifs.get();

		for (int i = 0; i < 8; ++i, buffer <<= 1) {
			current_code += (buffer & 128) == 0 ? '0' : '1';

			auto search_result = codes_char.find(current_code);

			if (search_result != codes_char.end()) {
				if (search_result->second != peseudo_eof) {
					ofs << (char)search_result->second;
					current_code.clear();
				} else {
					peseudo_eof_found = true;
					break;
				}
			}
		}
	}

	ofs.close();
	ifs.close();
}

void uncompress_file(string file_name) {
	ifs.open(file_name);

	if (!ifs.is_open()) {
		cout << "文件打开失败！" << endl;
	} else {
		cout << "解压中......" << endl << endl;

		unordered_map<string, int> codes_char = get_codes_char();

		write_into_uncompressed_file(codes_char, file_name);
	}
}

int main(int argc, char **argv) {
	int option;

	cout << "0 表示压缩, 1 表示解压。" << endl
		<< "请输入 0 或 1:" << endl;
	cin >> option;
	cout << endl;

	if (option == 0) {
		chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
		compress_file("cacm.all");
		chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
		chrono::duration<double> dur = chrono::duration_cast<chrono::duration<double>>(end - start);
		cout << "压缩时长：" << dur.count() << "s" << endl;
	} else if (option == 1) {
		chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
		uncompress_file("cacm.bin");
		chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
		chrono::duration<double> dur = chrono::duration_cast<chrono::duration<double>>(end - start);
		cout << "解压时长：" << dur.count() << "s" << endl;
	} else {
        cout << "输入有误!" << endl;
    }

	return 0;
}
