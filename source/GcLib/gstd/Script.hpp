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

#include "../pch.h"

#pragma warning (disable:4786)	//STL Warning抑止
#pragma warning (disable:4018)	//signed と unsigned の数値を比較
#pragma warning (disable:4244)	//double' から 'float' に変換

#include "Logger.hpp"
#include "LightweightVector.hpp"

//-------------------------------------SCRIPT OPTIONS-------------------------------------

//Forbid duplicated variable declaration
#define __SCRIPT_H__CHECK_NO_DUPLICATED

//Use inline operations
#define __SCRIPT_H__INLINE_OPERATION

//Discards empty 'if' and 'loop' blocks
#define __SCRIPT_H__BLOCK_OPTIMIZE

//----------------------------------------------------------------------------------------

//-------- 汎用
namespace gstd {
	//-------- ここから

	class type_data {
	public:
		enum class type_kind : uint8_t {
			tk_real, tk_char, tk_boolean, tk_array
		};

		type_data(type_kind k, type_data* t = nullptr) : kind(k), element(t) {}
		type_data(type_data const& source) : kind(source.kind), element(source.element) {}

		//デストラクタはデフォルトに任せる

		type_kind get_kind() { return kind; }
		type_data* get_element() { return element; }

		friend bool operator==(const type_data& ld, const type_data& rd) {
			if (ld.kind != rd.kind) return false;

			//Same element or both null
			if (ld.element == rd.element) return true;
			//Either null
			else if (ld.element == nullptr || rd.element == nullptr) return false;

			return (*ld.element == *rd.element);
		}
		bool operator<(const type_data& other) const {
			if (kind != other.kind) return ((uint8_t)kind < (uint8_t)other.kind);
			if (element == nullptr || other.element == nullptr) return false;
			return (*element) < (*other.element);
		}
	private:
		type_kind kind;
		type_data* element;
	};

	class value {
	public:
		value() : data(nullptr) {}
		value(type_data* t, double v) {
			data = std::shared_ptr<body>(new body);
			data->type = t;
			data->real_value = v;
		}
		value(type_data* t, wchar_t v) {
			data = std::shared_ptr<body>(new body);
			data->type = t;
			data->char_value = v;
		}
		value(type_data* t, bool v) {
			data = std::shared_ptr<body>(new body);
			data->type = t;
			data->boolean_value = v;
		}
		value(type_data* t, std::wstring v);
		value(value const& source) {
			data = source.data;
		}

		~value() {
			release();
		}

		value& operator=(value const& source) {
			data = source.data;
			return *this;
		}

		bool has_data() const {
			return data != nullptr;
		}

		void set(type_data* t, double v) {
			unique();
			data->type = t;
			data->real_value = v;
		}
		void set(type_data* t, bool v) {
			unique();
			data->type = t;
			data->boolean_value = v;
		}
		void set(type_data* t, std::vector<value>& v) {
			unique();
			data->type = t;
			data->array_value = v;
		}

		void append(type_data* t, value const& x);
		void concatenate(value const& x);

		double as_real() const;
		wchar_t as_char() const;
		bool as_boolean() const;
		std::wstring as_string() const;
		size_t length_as_array() const;
		value const& index_as_array(size_t i) const;
		value& index_as_array(size_t i);
		type_data* get_type() const;

		void unique() const {
			if (data == nullptr) {
				data = std::shared_ptr<body>(new body);
				data->type = nullptr;
			}
			else if (!data.unique()) {
				body* newData = new body(*data);
				data = std::shared_ptr<body>(newData);
			}
		}

		void overwrite(const value& source);	//危険！外から呼ぶな

		static value new_from(const value& source);
	private:
		inline void release() {
			if (data) data.reset();
		}
		struct body {
			type_data* type = nullptr;
			std::vector<value> array_value;

			union {
				double real_value = 0.0;
				wchar_t char_value;
				bool boolean_value;
			};
		};

		mutable std::shared_ptr<body> data;
	};

	class script_engine;
	class script_machine;

	typedef value (*callback)(script_machine* machine, int argc, value const* argv);

	//Breaks IntelliSense for some reason
#define DNH_FUNCAPI_DECL_(fn) static gstd::value fn (gstd::script_machine* machine, int argc, const gstd::value* argv)
#define DNH_FUNCAPI_(fn) gstd::value fn (gstd::script_machine* machine, int argc, const gstd::value* argv)

	struct function {
		char const* name;
		callback func;
		unsigned arguments;
	};

	class script_type_manager {
		static script_type_manager* base_;
	public:
		script_type_manager();

		type_data* get_real_type() {
			return const_cast<type_data*>(&*real_type);
		}
		type_data* get_char_type() {
			return const_cast<type_data*>(&*char_type);
		}
		type_data* get_boolean_type() {
			return const_cast<type_data*>(&*boolean_type);
		}
		type_data* get_string_type() {
			return const_cast<type_data*>(&*string_type);
		}
		type_data* get_array_type(type_data* element);

		static script_type_manager* get_instance() { return base_; }
	private:
		script_type_manager(script_type_manager const&);

		std::set<type_data> types;
		std::set<type_data>::iterator real_type;
		std::set<type_data>::iterator char_type;
		std::set<type_data>::iterator boolean_type;
		std::set<type_data>::iterator string_type;
	};

	class script_engine {
	public:
		script_engine(std::string const& source, int funcc, function const* funcv);
		script_engine(std::vector<char> const& source, int funcc, function const* funcv);
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
			return script_type_manager::get_instance();
		}

		//compatibility
		type_data* get_real_type() {
			return get_type_manager()->get_real_type();
		}
		type_data* get_char_type() {
			return get_type_manager()->get_char_type();
		}
		type_data* get_boolean_type() {
			return get_type_manager()->get_boolean_type();
		}
		type_data* get_array_type(type_data* element) {
			return get_type_manager()->get_array_type(element);
		}
		type_data* get_string_type() {
			return get_type_manager()->get_string_type();
		}

#ifndef _MSC_VER
	private:
#endif

		//コピー、代入演算子の自動生成を無効に
		script_engine(script_engine const& source);
		script_engine& operator=(script_engine const& source);

		//エラー
		bool error;
		std::wstring error_message;
		int error_line;

		//中間コード
		enum class command_kind : uint8_t {
			pc_var_alloc, pc_assign, pc_assign_writable, pc_break_loop, pc_break_routine, 
			pc_call, pc_call_and_push_result, pc_call_and_assign,
			pc_case_begin, pc_case_end, pc_case_if, pc_case_if_not, pc_case_next, 
			pc_compare_e, pc_compare_g, pc_compare_ge, pc_compare_l,
			pc_compare_le, pc_compare_ne, 
			pc_dup_n,
			pc_for, pc_for_each_and_push_first,
			pc_compare_and_loop_ascent, pc_compare_and_loop_descent,
			pc_loop_count, pc_loop_if, pc_loop_continue, pc_continue_marker, pc_loop_back,
			pc_construct_array,
			pc_pop, pc_push_value, pc_push_variable, pc_push_variable_writable, pc_swap, pc_yield, pc_wait,

#ifdef __SCRIPT_H__INLINE_OPERATION
			//Inline operations
			pc_inline_inc, pc_inline_dec,
			pc_inline_add_asi, pc_inline_sub_asi, pc_inline_mul_asi, pc_inline_div_asi, pc_inline_mod_asi, pc_inline_pow_asi,
			pc_inline_neg, pc_inline_not, pc_inline_abs,
			pc_inline_add, pc_inline_sub, pc_inline_mul, pc_inline_div, pc_inline_mod, pc_inline_pow,
			pc_inline_app, pc_inline_cat,
			pc_inline_cmp_e, pc_inline_cmp_g, pc_inline_cmp_ge, pc_inline_cmp_l, pc_inline_cmp_le, pc_inline_cmp_ne,
#endif
		};

		struct block;

		struct code {
			command_kind command;
			int line;	//ソースコード上の行
			value data;	//pc_push_valueでpushするデータ

#ifdef _DEBUG
			std::string var_name;	//For assign/push_variable
#endif

			union {
				struct {	//assign/push_variable
					int level;
					size_t variable;
				};
				struct {	//call/call_and_push_result
					block* sub;
					size_t arguments;
				};
				struct {	//loop_back
					size_t ip;
				};
				struct {	//call_and_assign
					block* sub;
					size_t arguments;
					size_t variable;
					int level;
				};
			};

			code() {}

			code(int the_line, command_kind the_command) : line(the_line), command(the_command) {}

			code(int the_line, command_kind the_command, int the_level, size_t the_variable, std::string the_name) : line(the_line),
				command(the_command), level(the_level), variable(the_variable)
			{
#ifdef _DEBUG
				var_name = the_name;
#endif
			}

			code(int the_line, command_kind the_command, block* the_sub, int the_arguments) : line(the_line), command(the_command), sub(the_sub),
				arguments(the_arguments) {
			}

			code(int the_line, command_kind the_command, size_t the_ip) : line(the_line), command(the_command), ip(the_ip) {}

			code(int the_line, command_kind the_command, value& the_data) : line(the_line), command(the_command), data(the_data) {}
		};

		enum class block_kind : uint8_t {
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
				level(the_level), arguments(0), name(), func(nullptr), codes(), kind(the_kind) {
			}
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

		void raise_error(std::wstring const& message) {
			error = true;
			error_message = message;
			finished = true;
		}
		void terminate(std::wstring const& message) {
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
		script_machine(script_machine const& source);
		script_machine& operator=(script_machine const& source);

		script_engine* engine;

		bool error;
		std::wstring error_message;
		int error_line;

		bool bTerminate;

		typedef script_vector<value> variables_t;
		typedef script_vector<value> stack_t;
		//typedef std::vector<value> variables_t;
		//typedef std::vector<value> stack_t;

		class environment {
		public:
			environment(std::shared_ptr<environment> parent, script_engine::block* b);
			~environment();

			std::shared_ptr<environment> parent;
			script_engine::block* sub;
			int ip;
			variables_t variables;
			stack_t stack;
			bool has_result;
			int waitCount;
		};
		using environment_ptr = std::shared_ptr<environment>;

		std::list<environment_ptr> call_start_parent_environment_list;

		std::list<environment_ptr> threads;
		std::list<environment_ptr>::iterator current_thread_index;
		bool finished;
		bool stopped;
		bool resuming;

		void yield() {
			if (current_thread_index == threads.begin())
				current_thread_index = std::prev(threads.end());
			else
				--current_thread_index;
		}

		void advance();
		value* find_variable_symbol(environment* current_env, script_engine::code* var_data);
	public:
		size_t get_thread_count() { return threads.size(); }
	};
	inline bool script_machine::has_event(std::string event_name) {
		assert(!error);
		return engine->events.find(event_name) != engine->events.end();
	}
	inline int script_machine::get_current_line() {
		environment_ptr current = *current_thread_index;
		script_engine::code* c = &(current->sub->codes[current->ip]);
		return c->line;
	}

	template<int num>
	class constant {
	public:
		static value func(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), (double)num);
		}
	};

	class mconstant {
	public:
		static value funcPi(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI);
		}
		static value funcPiX2(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_X2);
		}
		static value funcPiX4(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_X4);
		}
		static value funcPi2(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_2);
		}
		static value funcPi4(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_PI_4);
		}
		static value func1Pi(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_1_PI);
		}
		static value func2Pi(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_2_PI);
		}
		static value funcSqrtPi(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRTP);
		}
		static value func1SqrtPi(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_1_SQRTP);
		}
		static value func2SqrtPi(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_2_SQRTP);
		}
		static value funcSqrt2(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRT2);
		}
		static value funcSqrt2_2(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRT2_2);
		}
		static value funcSqrt2_X2(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_SQRT2_X2);
		}
		static value funcEuler(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_E);
		}
		static value funcLog2Euler(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LOG2E);
		}
		static value funcLog10Euler(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LOG10E);
		}
		static value funcLn2(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LN2);
		}
		static value funcLn10(script_machine* machine, int argc, value const* argv) {
			return value(machine->get_engine()->get_real_type(), GM_LN10);
		}
	};
}
