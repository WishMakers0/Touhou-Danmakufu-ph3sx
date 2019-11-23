/* ---------------------------------- */
/* 2004..2005 YT、mkm(本体担当ですよ)*/
/* このソースコードは煮るなり焼くなり好きにしろライセンスの元で配布します。*/
/* A-3、A-4に従い、このソースを組み込んだ.exeにはライセンスは適用されません。*/
/* ---------------------------------- */
/* NYSL Version 0.9982 */
/* A. 本ソフトウェアは Everyone'sWare です。このソフトを手にした一人一人が、*/
/*    ご自分の作ったものを扱うのと同じように、自由に利用することが出来ます。*/
/* A-1. フリーウェアです。作者からは使用料等を要求しません。*/
/* A-2. 有料無料や媒体の如何を問わず、自由に転載・再配布できます。*/
/* A-3. いかなる種類の 改変・他プログラムでの利用 を行っても構いません。*/
/* A-4. 変更したものや部分的に使用したものは、あなたのものになります。*/
/*      公開する場合は、あなたの名前の下で行って下さい。*/
/* B. このソフトを利用することによって生じた損害等について、作者は */
/*    責任を負わないものとします。各自の責任においてご利用下さい。*/
/* C. 著作者人格権は ○○○○ に帰属します。著作権は放棄します。*/
/* D. 以上の３項は、ソース・実行バイナリの双方に適用されます。 */
/* ---------------------------------- */

#pragma once

#pragma warning (disable:4786)	//STL Warning抑止
#pragma warning (disable:4018)	//signed と unsigned の数値を比較
#pragma warning (disable:4244)	//double' から 'float' に変換

#include <list>
#include <vector>
#include <string>
#include <map>

#include "Logger.hpp"


//重複宣言チェックをしない
//#define __SCRIPT_H__NO_CHECK_DUPLICATED

//-------- 汎用
namespace gstd {
	std::string to_mbcs(const std::wstring& s);
	std::wstring to_wide(const std::string& s);

	//-------- ここから

	class type_data {
	public:
		enum type_kind {
			tk_real, tk_char, tk_boolean, tk_array
		};

		type_data(type_kind k, type_data* t = nullptr) : kind(k), element(t) {}

		type_data(const type_data& source) : kind(source.kind), element(source.element) {}

		//デストラクタはデフォルトに任せる

		type_kind get_kind() {
			return kind;
		}

		type_data* get_element() {
			return element;
		}

	private:
		type_kind kind;
		type_data* element;

		type_data& operator=(const type_data& source);
	};

	class value {
	public:
		value() : data(nullptr) {}
		value(type_data* t, double v) {
			data = new body();
			data->ref_count = 1;
			data->type = t;
			data->real_value = v;
		}
		value(type_data* t, wchar_t v) {
			data = new body();
			data->ref_count = 1;
			data->type = t;
			data->char_value = v;
		}
		value(type_data* t, bool v) {
			data = new body();
			data->ref_count = 1;
			data->type = t;
			data->boolean_value = v;
		}
		value(type_data* t, std::wstring v);
		value(const value& source) {
			data = source.data;
			if (data != nullptr)
				++(data->ref_count);
		}

		~value() {
			release();
		}

		value& operator = (const value& source) {
			if (source.data != nullptr) {
				++(source.data->ref_count);
			}
			release();
			data = source.data;
			return *this;
		}

		bool has_data() const {
			return data != nullptr;
		}

		void set(type_data * t, double v) {
			unique();
			data->type = t;
			data->real_value = v;
		}

		void set(type_data * t, bool v) {
			unique();
			data->type = t;
			data->boolean_value = v;
		}

		void append(type_data* t, const value& x);
		void concatenate(const value& x);

		double as_real() const;
		wchar_t as_char() const;
		bool as_boolean() const;
		std::wstring as_string() const;
		unsigned length_as_array() const;
		const value& index_as_array(size_t i) const;
		value& index_as_array(size_t i);
		type_data * get_type() const;

		void unique() const {
			if (data == nullptr) {
				data = new body();
				data->ref_count = 1;
				data->type = nullptr;
			}
			else if (data->ref_count > 1) {
				--(data->ref_count);
				data = new body(*data);
				data->ref_count = 1;
			}
		}

		void overwrite(const value& source);	//危険！外から呼ぶな

	private:
		inline void release() {
			if (data != nullptr) {
				--(data->ref_count);
				if (data->ref_count == 0) {
					delete data;
				}
			}
		}
		struct body {
			int ref_count;
			type_data* type;
			std::vector<value> array_value;

			union {
				double real_value;
				wchar_t char_value;
				bool boolean_value;
			};
		};

		mutable body* data;
	};

	class script_engine;
	class script_machine;

	typedef value(*callback) (script_machine* machine, int argc, const value* argv);

	struct function {
		const char* name;
		callback func;
		unsigned arguments;
	};

	class script_type_manager {
	public:
		script_type_manager();

		type_data* get_real_type() {
			return real_type;
		}

		type_data* get_char_type() {
			return char_type;
		}

		type_data* get_boolean_type() {
			return boolean_type;
		}

		type_data* get_string_type() {
			return string_type;
		}

		type_data* get_array_type(type_data* element);
	private:
		script_type_manager(const script_type_manager&);
		script_type_manager& operator=(const script_type_manager& source);

		std::list<type_data> types;	//中身のポインタを使うのでアドレスが変わらないようにlist
		type_data* real_type;
		type_data* char_type;
		type_data* boolean_type;
		type_data* string_type;
	};

	class script_engine {
	public:
		script_engine(script_type_manager* a_type_manager, const std::string& source, int funcc, const function* funcv);
		script_engine(script_type_manager* a_type_manager, const std::vector<char>& source, int funcc, const function* funcv);
		virtual ~script_engine();

		void* data;	//クライアント用空間

		bool get_error() {
			return error;
		}

		std::wstring& get_error_message() {
			return error_message;
		}

		int get_error_line() {
			return error_line;
		}

		script_type_manager* get_type_manager() {
			return type_manager;
		}

		//compatibility
		type_data* get_real_type() {
			return type_manager->get_real_type();
		}

		type_data* get_char_type() {
			return type_manager->get_char_type();
		}

		type_data* get_boolean_type() {
			return type_manager->get_boolean_type();
		}

		type_data* get_array_type(type_data* element) {
			return type_manager->get_array_type(element);
		}

		type_data* get_string_type() {
			return type_manager->get_string_type();
		}

#ifndef _MSC_VER
	private:
#endif

		//コピー、代入演算子の自動生成を無効に
		script_engine(const script_engine& source);
		script_engine& operator=(const script_engine& source);

		//エラー
		bool error;
		std::wstring error_message;
		int error_line;

		//型
		script_type_manager * type_manager;

		//中間コード
		enum command_kind {
			pc_assign, pc_assign_writable, pc_break_loop, pc_break_routine, pc_call, pc_call_and_push_result, pc_case_begin,
			pc_case_end, pc_case_if, pc_case_if_not, pc_case_next, pc_compare_e, pc_compare_g, pc_compare_ge, pc_compare_l,
			pc_compare_le, pc_compare_ne, pc_dup, pc_dup2, 
			pc_loop_ascent, pc_loop_back, pc_loop_count, pc_loop_descent, pc_loop_if, /*pc_loop_continue,*/
			pc_pop, pc_push_value, pc_push_variable, pc_push_variable_writable, pc_swap, pc_yield, pc_wait
		};

		struct block;

		struct code {
			command_kind command;
			int line;	//ソースコード上の行
			value data;	//pc_push_valueでpushするデータ

			union {
				struct {
					int level;	//assign/push_variableの変数の環境位置
					unsigned variable;	//assign/push_variableの変数のインデックス
				};
				struct {
					block* sub;	//call/call_and_push_resultの飛び先
					unsigned arguments;	//call/call_and_push_resultの引数の数
				};
				struct {
					int ip;	//loop_backの戻り先
				};
			};

			code() {}

			code(int the_line, command_kind the_command) : line(the_line), command(the_command) {}

			code(int the_line, command_kind the_command, int the_level, unsigned the_variable) : line(the_line), command(the_command), level(the_level), variable(the_variable) {}

			code(int the_line, command_kind the_command, block* the_sub, int the_arguments) : line(the_line), command(the_command), sub(the_sub),
				arguments(the_arguments) {}

			code(int the_line, command_kind the_command, int the_ip) : line(the_line), command(the_command), ip(the_ip) {}

			code(int the_line, command_kind the_command, value& the_data) : line(the_line), command(the_command), data(the_data) {}
		};

		enum block_kind {
			bk_normal, bk_loop, bk_sub, bk_function, bk_microthread
		};

		friend struct block;

		typedef std::vector<code> codes_t;

		struct block {
			int level;
			int arguments;
			std::string name;
			callback func;
			codes_t codes;
			block_kind kind;

			block(int the_level, block_kind the_kind) : 
				level(the_level), arguments(0), name(), func(nullptr), codes(), kind(the_kind) {}
		};

		std::list<block> blocks;	//中身のポインタを使うのでアドレスが変わらないようにlist
		block* main_block;
		std::map<std::string, block*> events;

		block* new_block(int level, block_kind kind) {
			block x(level, kind);
			return &*blocks.insert(blocks.end(), x);
		}

		friend class parser;
		friend class script_machine;
	};

	class script_machine {
	public:
		script_machine(script_engine* the_engine);
		virtual ~script_machine();

		void* data;	//クライアント用空間

		void run();
		void call(std::string event_name);
		void resume();

		void stop() {
			finished = true;
			stopped = true;
		}

		bool get_stopped() {
			return stopped;
		}

		bool get_resuming() {
			return resuming;
		}

		bool get_error() {
			return error;
		}

		std::wstring& get_error_message() {
			return error_message;
		}

		int get_error_line() {
			return error_line;
		}

		void raise_error(const std::wstring& message) {
			error = true;
			error_message = message;
			finished = true;
		}
		void terminate(const std::wstring& message) {
			bTerminate = true;
			error = true;
			error_message = message;
			finished = true;
		}

		script_engine* get_engine() {
			return engine;
		}

		bool has_event(std::string event_name);

		int get_current_line();

	private:
		script_machine();
		script_machine(const script_machine& source);
		script_machine& operator=(const script_machine& source);

		script_engine* engine;

		bool error;
		std::wstring error_message;
		int error_line;

		bool bTerminate;

		typedef std::vector<value> variables_t;
		typedef std::vector<value> stack_t;

		struct environment {
			environment* pred;	//双方向リンクリスト
			environment* succ;
			environment* parent;
			int ref_count;
			script_engine::block* sub;
			unsigned ip;
			variables_t variables;
			stack_t stack;
			bool has_result;
			int waitCount;
		};

		std::list<environment*> call_start_parent_environment_list;
		environment* first_using_environment;
		environment* last_using_environment;
		environment* first_garbage_environment;
		environment* last_garbage_environment;
		environment* new_environment(environment* parent, script_engine::block* b);
		void dispose_environment(environment* object);

		typedef std::vector<environment*> threads_t;

		threads_t threads;
		size_t current_thread_index;
		bool finished;
		bool stopped;
		bool resuming;

		void yield() {
			if (current_thread_index > 0)
				--current_thread_index;
			else
				current_thread_index = threads.size() - 1;
		}

		void advance();

	public:
		int get_thread_count() { return threads.size(); }
	};

	template<int num>
	class constant {
	public:
		static value func(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), (double)num);
		}
	};

	class mconstant {
	public:
		static value funcPi(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI);
		}
		static value funcPiX2(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_X2);
		}
		static value funcPiX4(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_X4);
		}
		static value funcPi2(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_2);
		}
		static value funcPi4(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_4);
		}
		static value func1Pi(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_1_PI);
		}
		static value func2Pi(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_2_PI);
		}
		static value funcSqrtPi(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRTP);
		}
		static value func1SqrtPi(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_1_SQRTP);
		}
		static value func2SqrtPi(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_2_SQRTP);
		}
		static value funcSqrt2(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRT2);
		}
		static value funcSqrt2_2(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRT2_2);
		}
		static value funcSqrt2_X2(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRT2_X2);
		}
		static value funcEuler(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_E);
		}
		static value funcLog2Euler(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LOG2E);
		}
		static value funcLog10Euler(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LOG10E);
		}
		static value funcLn2(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LN2);
		}
		static value funcLn10(script_machine* machine, int argc, const value* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LN10);
		}
	};
}
