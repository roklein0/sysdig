//
// k8s_node_handler.h
//

#pragma once

#include "json/json.h"
#include "sinsp_auth.h"
#include "k8s_handler.h"
#include "k8s_state.h"

class sinsp;

class k8s_node_handler : public k8s_handler
{
public:
	typedef std::vector<std::string>     uri_list_t;
	typedef std::shared_ptr<Json::Value> json_ptr_t;
	typedef sinsp_ssl::ptr_t             ssl_ptr_t;
	typedef sinsp_bearer_token::ptr_t    bt_ptr_t;

	k8s_node_handler(k8s_state_t& state,
		std::string url,
		const std::string& http_version = "1.0",
		ssl_ptr_t ssl = 0,
		bt_ptr_t bt = 0);

	~k8s_node_handler();

	//bool is_delegated(bool trace = false);

private:
	static std::string EVENT_FILTER;
	static std::string STATE_FILTER;

	virtual void handle_json(Json::Value&& root);
	void handle_node(const msg_data& data, const Json::Value& n);
	void log_error(const Json::Value& root, const std::string& comp);

	k8s_state_t& m_state;
};
