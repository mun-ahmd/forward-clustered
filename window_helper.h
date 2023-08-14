#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>

class InputManager;

struct UserPointerObject {
	InputManager* inputManager;
	UserPointerObject(InputManager* inputManager) : inputManager(inputManager) {}
};

InputManager* getInputManager(GLFWwindow* window) {
	return static_cast<UserPointerObject*>(glfwGetWindowUserPointer(window))->inputManager;
}

typedef std::function<void(int, int)> InputFunction;

class InputManager {
private:
	GLFWwindow* window;
	std::unordered_map<int, std::unordered_map<std::string, InputFunction>> action_mapping;
	std::unordered_map<int, int> key_pressed;

public:
	void trigger(int key, int new_state, int mods = 0) {
		auto old_state = this->key_pressed.find(key);
		if (old_state == this->key_pressed.end() || old_state->second != new_state) {
			for (auto& actionItem : this->action_mapping[key]) {
				actionItem.second(new_state, mods);
			}
		}
		this->key_pressed[key] = new_state;
	}

	void addCallback(int key, std::string name, InputFunction func) {
		auto keyMap = this->action_mapping.find(key);
		if (keyMap == this->action_mapping.end()) {
			this->action_mapping[key] = {};
		}
		auto& insideMap = this->action_mapping[key];
		insideMap[name] = func;
	}

	InputManager(GLFWwindow* window) : window(window) {
		GLFWkeyfun callback = [](GLFWwindow* w, int key, int scancode, int action, int mods) {
			InputManager* manager = getInputManager(w);
			manager->trigger(key, action, mods);
		};
		glfwSetKeyCallback(window, callback);
	}

};

UserPointerObject* createUserPointerObject(GLFWwindow* window) {
	InputManager* imanager = new InputManager(window);
	UserPointerObject* obj = new UserPointerObject(imanager);
	return obj;
}

void destroyUserPointerObject(GLFWwindow* window) {
	auto obj = static_cast<UserPointerObject*>(glfwGetWindowUserPointer(window));
	delete obj->inputManager;
	delete obj;
	glfwSetWindowUserPointer(window, NULL);
}

GLFWwindow* createWindow(int width, int height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(window, (void*)createUserPointerObject(window));

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	return window;
}

void destroyWindow(GLFWwindow* window) {
	destroyUserPointerObject(window);
	glfwDestroyWindow(window);
	glfwTerminate();
}