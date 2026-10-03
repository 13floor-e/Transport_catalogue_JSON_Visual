#include "json_builder.h"

using namespace json;

Builder::DictKeyContext Builder::Key(std::string key) {
	if (nodes_stack_.empty()) {
		throw std::logic_error("Invalid context: no open container");
	}

	Node* current_node = nodes_stack_.back();

	if (!std::holds_alternative<Dict>(current_node->GetValue())) {
		throw std::logic_error("Key() can only be called inside a Dict");
	}

	Dict& dict = std::get<Dict>(current_node->GetValue());

	Node& value_slot = dict[std::move(key)];

	nodes_stack_.push_back(&value_slot);

	return DictKeyContext(*this);
}


Builder::DictItemContext json::Builder::ValueImpl(json::Value val)
{
	if (nodes_stack_.empty()) {
		throw std::logic_error(
			"Cannot add value after Build()"
		);
	}

	Node* current_node = nodes_stack_.back();
	json::Value& current_value = current_node->GetValue();

	// Добавление значения в массив
	if (std::holds_alternative<Array>(current_value)) {
		std::get<Array>(current_value).emplace_back(std::move(val));

		//arr.emplace_back(std::move(val));
		return DictItemContext(*this);
	}
	// Заполнение пустого Node.
	// Это корень или значение после Key().
	else if (std::holds_alternative<std::nullptr_t>(current_value)) {
		current_value = std::move(val);

		// Если это значение словаря, после его заполнения
		// возвращаемся к родительскому контейнеру.
		//if (nodes_stack_.size() >= 1) {
		nodes_stack_.pop_back();
		//}
		return DictItemContext(*this);
	}
	else {
		throw std::logic_error(
			"Value cannot be added here"
		);
	}

}


Builder::DictItemContext Builder::StartDict() {
	if (nodes_stack_.empty()) {
		throw std::logic_error("Cannot add dict after Build()");
	}

	Node* current_node = nodes_stack_.back();
	json::Value& current_value = current_node->GetValue();

	// В текущий массив добавляем новый словарь
	if (std::holds_alternative<Array>(current_value)) {
		Array& current_array =
			std::get<Array>(current_value);

		Node& new_node =
			current_array.emplace_back(Dict{});

		nodes_stack_.push_back(&new_node);
	}
	// Пустой Node превращаем в словарь.
	// Это может быть корень или значение после Key().
	else if (std::holds_alternative<std::nullptr_t>(current_value)) {
		current_node->GetValue() = Dict{};
		//nodes_stack_.push_back(current_node);
	}
	else {
		throw std::logic_error(
			"StartDict called in invalid context"
		);
	}
	//	std::cerr << "StartDict: Stack size = " << nodes_stack_.size() << "\n";
	return DictItemContext(*this);
}

Builder::ArrayItemContext Builder::StartArray() {
	Array arr = {};

	if (nodes_stack_.empty()) {
		throw std::logic_error("Cannot add value after Build()");
	}
	Node* current_node = nodes_stack_.back();
	const auto& current_val = current_node->GetValue();

	if (std::holds_alternative<Array>(current_val)) {
		auto& new_array = std::get<Array>(current_node->GetValue());
		Node& newly_added_node = new_array.emplace_back(std::move(arr));
		nodes_stack_.push_back(&newly_added_node);
	}
	else if (std::holds_alternative<std::nullptr_t>(current_val)) {
		current_node->GetValue() = Array{};
		//nodes_stack_.push_back(current_node);

	}
	else {
		throw std::logic_error("StartArray called in invalid context");
	}
	return ArrayItemContext(*this);
}

Builder::BaseContext Builder::EndDict() {

	//// --- НАЧАЛО ОТЛАДКИ ---
	//std::cerr << "[DEBUG] EndDict called. Stack size: " << nodes_stack_.size() << "\n";
	//if (!nodes_stack_.empty()) {
	//	const auto& top_val = nodes_stack_.back()->GetValue();
	//	if (std::holds_alternative<Dict>(top_val)) std::cerr << "[DEBUG] Top is DICT\n";
	//	else if (std::holds_alternative<Array>(top_val)) std::cerr << "[DEBUG] Top is ARRAY\n";
	//	else std::cerr << "[DEBUG] Top is EMPTY/VALUE\n";
	//}
	//else {
	//	std::cerr << "[DEBUG] Stack is EMPTY!\n";
	//}
	//// --- КОНЕЦ ОТЛАДКИ ---

	if (nodes_stack_.empty()) {
		throw std::logic_error("An attempt to close Dict when nothing is open");
	}

	Node* current_node = nodes_stack_.back();
	const auto& current_val = current_node->GetValue();


	if (!std::holds_alternative<Dict>(current_val)) {
		throw std::logic_error("EndDict called in invalid context");
	}
	nodes_stack_.pop_back();
	return BaseContext(*this);
}

Builder::ArrayItemContext json::Builder::EndDictInArray()
{
	if (nodes_stack_.empty()) {
		throw std::logic_error("An attempt to close Dict when nothing is open");
	}

	Node* current_node = nodes_stack_.back();
	const auto& current_val = current_node->GetValue();
	if (!std::holds_alternative<Dict>(current_val)) {
		throw std::logic_error("EndDict called in invalid context");
	}
	nodes_stack_.pop_back();
	return ArrayItemContext(*this);
}

Builder::BaseContext Builder::EndArray() {
	if (nodes_stack_.empty()) {
		throw std::logic_error("An attempt to close Array when nothing is open");
	}
	Node* current_node = nodes_stack_.back();
	const auto& current_val = current_node->GetValue();
	if (!std::holds_alternative<Array>(current_val)) {
		throw std::logic_error("EndArray called in invalid context");
	}
	nodes_stack_.pop_back();
	return BaseContext(*this);
}

json::Builder::BaseContext Builder::Value(json::Value val)
{
	if (nodes_stack_.empty()) {
		throw std::logic_error("Invalid context: stack is empty");
	}

	Node* current_node = nodes_stack_.back();
	json::Value& current_value = current_node->GetValue();

	if (std::holds_alternative<Array>(current_value)) {
		// Случай: мы внутри массива. Добавляем элемент.
		// Стек НЕ трогаем, мы остаемся внутри этого же массива.
		std::get<Array>(current_value).emplace_back(std::move(val));
	}
	else if (std::holds_alternative<std::nullptr_t>(current_value)) {
		// Случай: корень или значение после ключа.
		current_value = std::move(val);
		nodes_stack_.pop_back();

	}
	else {
		throw std::logic_error("Value cannot be added here");
	}

	return BaseContext(*this);
}


Node Builder::Build() {
	if (!nodes_stack_.empty()) {
		throw std::logic_error("JSON is not finalized: there are unclosed containers");
	}

	return std::move(root_);
}