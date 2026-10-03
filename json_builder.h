#pragma once
#include "json.h"
#include <optional>


namespace json {

	class Builder
	{

	public:


		class BaseContext;


		class DictItemContext;
		class ArrayItemContext;
		class DictKeyContext;


		Builder()
			: root_(nullptr)
			, nodes_stack_{ &root_ } {
		}

		DictKeyContext Key(std::string key);

		DictItemContext ValueImpl(json::Value val);

		BaseContext Value(json::Value val);

		//ArrayItemContext EndArrayD(
		DictItemContext StartDict();

		ArrayItemContext StartArray();

		BaseContext EndDict();

		ArrayItemContext EndDictInArray();

		BaseContext EndArray();


		json::Node Build();

	private:

		Builder(const Builder&) = delete;
		Builder& operator=(const Builder&) = delete;

		Node root_;
		std::vector<json::Node*> nodes_stack_; // Стек для отслеживания текущего места
		std::optional<std::string> current_key_; // текущий ключ
		//};
	public:
		class BaseContext {
		public:
			explicit BaseContext(Builder& b) : builder_(b) {}

			DictKeyContext Key(std::string key) { return builder_.Key(key); }

			BaseContext Value(json::Value val) { return builder_.Value(std::move(val)); }


			DictItemContext StartDict() { return builder_.StartDict(); }

			BaseContext EndDict() { return builder_.EndDict(); }

			BaseContext EndArray() { return builder_.EndArray(); }

			Node Build() { return builder_.Build(); }
		protected:
			Builder& builder_;
		}; // BaseContext


		class DictItemContext : public BaseContext {
		public:
			explicit DictItemContext(Builder& b) :BaseContext(b) {}

			DictKeyContext Key(std::string key) { return builder_.Key(key); }
			BaseContext EndDict() { return builder_.EndDict(); }
			// Запрещаем всё остальное
			BaseContext EndArray() = delete;
			DictItemContext Value(json::Value) = delete;
			ArrayItemContext StartArray() = delete;
			DictItemContext StartDict() = delete;

			Node Build() = delete;
		}; // DictItemContext


		class ArrayItemContext : public BaseContext {
		public:
			explicit ArrayItemContext(Builder& b) :BaseContext(b) {}

			ArrayItemContext Value(json::Value val) {
				if (builder_.nodes_stack_.empty()) {
					throw std::logic_error(
						"Cannot add value after Build()"
					);
				}

				Node* current_node = builder_.nodes_stack_.back();
				json::Value& current_value = current_node->GetValue();

				// Добавление значения в массив
				if (std::holds_alternative<Array>(current_value)) {
					Array& arr =
						std::get<Array>(current_value);

					arr.emplace_back(std::move(val));
				}
				// Заполнение пустого Node.
				// Это корень или значение после Key().
				else if (std::holds_alternative<std::nullptr_t>(current_value)) {
					current_value = std::move(val);

				}
				else {
					throw std::logic_error(
						"Value cannot be added here"
					);
				}

				return ArrayItemContext(*this);
			}

			DictItemContext StartDict() { return builder_.StartDict(); }
			ArrayItemContext StartArray() { return builder_.StartArray(); }
			BaseContext EndArray() { return builder_.EndArray(); }

			DictKeyContext Key(std::string key) = delete;
			BaseContext EndDict() = delete;
			Node Build() = delete;

		}; // ArrayItemContext

		class DictKeyContext : public BaseContext {
		public:
			explicit DictKeyContext(Builder& b) : BaseContext(b) {}

			// Разрешенные действия
			DictItemContext Value(json::Value val) { return builder_.ValueImpl(val); }
			DictItemContext StartDict() { return builder_.StartDict(); }
			ArrayItemContext StartArray() { return builder_.StartArray(); }

			// ЗАПРЕЩЕННЫЕ действия (удаляем их, чтобы код не компилировался)
			DictKeyContext Key(std::string key) = delete;      
			BaseContext EndDict() = delete;                
			BaseContext EndArray() = delete;               
			Node Build() = delete;

		}; //DictKeyContext

	};
} // namespace json

