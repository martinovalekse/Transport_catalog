#pragma once

#include "json.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
namespace json {

class Builder {
	class DictItemContext;
	class ArrayItemContext;
	class KeyItemContext;
	class ValueAfterKeyItemContext;
	class ValueAfterValueItemContext;

	class FunctionRelocation {
	public:
		FunctionRelocation(Builder &link) :
				builder_(link) {
		}

		DictItemContext StartDict() {
			return builder_.StartDict();
		}
		FunctionRelocation Value(Node::Value item) {
			return builder_.Value(item);
		}
		KeyItemContext Key(std::string key) {
			return builder_.Key(key);
		}
		ArrayItemContext StartArray() {
			return builder_.StartArray();
		}
		FunctionRelocation EndArray() {
			return builder_.EndArray();
		}
		FunctionRelocation EndDict() {
			return builder_.EndDict();
		}
		json::Node Build() {
			return builder_.Build();
		}

		Builder &builder_;
	};

	class DictItemContext: public FunctionRelocation {
	public:
		DictItemContext(Builder &link) :
				FunctionRelocation(link) {
		}

		ArrayItemContext StartArray() = delete;
		DictItemContext StartDict() = delete;
		FunctionRelocation EndArray() = delete;
		FunctionRelocation Value(Node::Value item) = delete;
		json::Node Build() = delete;
	};

	class ArrayItemContext: public FunctionRelocation {
	public:
		ArrayItemContext(Builder &link) :
				FunctionRelocation(link) {
		}

		KeyItemContext Key(std::string key) = delete;
		FunctionRelocation EndDict() = delete;
		json::Node Build() = delete;
		ValueAfterValueItemContext Value(Node::Value item);
	};

	class KeyItemContext: public FunctionRelocation {
	public:
		KeyItemContext(Builder &link) :
				FunctionRelocation(link) {
		}

		KeyItemContext Key(std::string key) = delete;
		FunctionRelocation EndDict() = delete;
		FunctionRelocation EndArray() = delete;
		json::Node Build() = delete;
		ValueAfterKeyItemContext Value(Node::Value item);
	};

	class ValueAfterKeyItemContext: public FunctionRelocation {
	public:
		ValueAfterKeyItemContext(Builder &link) :
				FunctionRelocation(link) {
		}

		FunctionRelocation Value(Node::Value item) = delete;
		ArrayItemContext StartArray() = delete;
		DictItemContext StartDict() = delete;
		FunctionRelocation EndArray() = delete;
		json::Node Build() = delete;
	};

	class ValueAfterValueItemContext: public FunctionRelocation {
	public:
		ValueAfterValueItemContext(Builder &link) :
				FunctionRelocation(link) {
		}
		ValueAfterValueItemContext Value(Node::Value item);
		KeyItemContext Key(std::string key) = delete;
		FunctionRelocation EndDict() = delete;
		json::Node Build() = delete;
	};

//------------------------ End of auxiliary compilation time classes ------------------------------------

	enum STATE {
		array, dict, key,
	};

public:
	ArrayItemContext StartArray();
	DictItemContext StartDict();
	KeyItemContext Key(std::string key);
	FunctionRelocation Value(Node::Value item);
	FunctionRelocation EndArray();
	FunctionRelocation EndDict();
	json::Node Build();

private:
	std::vector<int> pos_;
	std::vector<STATE> last_states_;
	std::vector<Node> nodes_stack_;
};

}// namespace json

