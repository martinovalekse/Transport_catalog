#include "json_builder.h"

namespace json {

Builder::ValueAfterKeyItemContext Builder::KeyItemContext::Value(Node::Value item) {
	if (!builder_.last_states_.empty()) {
		if (builder_.last_states_.back() == STATE::key
			|| (builder_.last_states_.back() == STATE::array || builder_.last_states_.back() == STATE::dict)) {
		} else {
			throw std::logic_error("just after dict");;
		}
		if (builder_.last_states_.back() == STATE::key) {
			builder_.last_states_.pop_back();
		}
	}
	Node new_node = std::visit([](auto val){ return Node(val); }, item);
	builder_.nodes_stack_.push_back(new_node);
	return builder_;
}

Builder::ValueAfterValueItemContext Builder::ArrayItemContext::Value(Node::Value item) {
	Node new_node = std::visit([](auto val){ return Node(val); }, item);
	builder_.nodes_stack_.push_back(new_node);
	return builder_;
}

//------------------------ Builder methods ------------------------------------

Builder::ArrayItemContext  Builder::StartArray() {
	if (!last_states_.empty()) {
		if (last_states_.back() == STATE::key || (last_states_.back() == STATE::array || last_states_.back() == STATE::dict)) {
		} else {
			throw std::logic_error("just after dict");;
		}
		if (last_states_.back() == STATE::key) {
			last_states_.pop_back();
		}
	}
	Node node{};
	nodes_stack_.push_back(move(node));
	last_states_.push_back(STATE::array);
	pos_.push_back(nodes_stack_.size() - 1);
	return *this;
}

Builder::DictItemContext Builder::StartDict() {
	if (!last_states_.empty()) {
		if (last_states_.back() == STATE::key || last_states_.back() == STATE::array) {
		} else {
			throw std::logic_error("just after dict");;
		}
		if (last_states_.back() == STATE::key) {
			last_states_.pop_back();
		}
	}
	Node node{};
	nodes_stack_.push_back(move(node));
	last_states_.push_back(STATE::dict);
	pos_.push_back(nodes_stack_.size() - 1);
	return *this;
}

Builder::KeyItemContext Builder::Key(std::string key)  {
	if (last_states_.empty()) {
		throw std::logic_error("big fucking error");
	}
	if ((last_states_.back() == STATE::key) || (last_states_.back() != STATE::dict) ) {
		throw std::logic_error("key after key");
	}
	nodes_stack_.emplace_back(key);
	last_states_.push_back(STATE::key);
	return *this;
}

Builder::FunctionRelocation  Builder::Value(Node::Value item) {
	if (!last_states_.empty()) {
		if (last_states_.back() == STATE::key || (last_states_.back() == STATE::array || last_states_.back() == STATE::dict)) {
		} else {
			throw std::logic_error("just after dict");;
		}
		if (last_states_.back() == STATE::key) {
			last_states_.pop_back();
		}
	}
	Node new_node = std::visit([](auto val){ return Node(val); }, item);
	nodes_stack_.push_back(new_node);
	return *this;
}

Builder::FunctionRelocation  Builder::EndArray() {
	(!last_states_.empty() && last_states_.back() == STATE::array) ? last_states_.pop_back() : throw std::logic_error("test array");
	Array something;
	for (int i = nodes_stack_.size() - 1; i >= 0; --i) {
		if (i == pos_.back()) { //nodes_stack_[i].GetValue().index() == 0
			pos_.pop_back();
			nodes_stack_.resize(i);
			break;
		} else {
			something.push_back(nodes_stack_[i]);
		}
	}
	reverse(something.begin(), something.end());
	nodes_stack_.push_back(move(something));
	return *this;
}

Builder::FunctionRelocation  Builder::EndDict() {
	(!last_states_.empty() && last_states_.back() == STATE::dict) ? last_states_.pop_back() : throw std::logic_error("test dict");
	Dict something{};
	for (int i = nodes_stack_.size() - 1; i >= 0; --i) {

		if (i == pos_.back()) { //nodes_stack_[i].GetValue().index() == 0
			pos_.pop_back();
			nodes_stack_.resize(i);
			break;
		} else {
			something.insert({nodes_stack_[i-1].AsString(), nodes_stack_[i] });
			--i;
		}
	}
	nodes_stack_.emplace_back(move(something));
	return *this;
}

json::Node Builder::Build() {
	if (!last_states_.empty() || nodes_stack_.empty()) {
		 throw std::logic_error("build not ready");
	}
	if (!last_states_.empty()) {
		 throw std::logic_error("build not start");
	}
	if (nodes_stack_.empty() || nodes_stack_.size() > 1) {
		throw std::logic_error("build not complete");
	}
	return *nodes_stack_.begin();
}

}// namespace json
