//
// k8s_handler.h
//

#pragma once

#include "json/json.h"
#include "socket_collector.h"
#include <unordered_set>

class sinsp;

class k8s_handler
{
public:
	typedef std::vector<std::string>        uri_list_t;
	typedef std::shared_ptr<Json::Value>    json_ptr_t;
	typedef sinsp_curl::ssl::ptr_t          ssl_ptr_t;
	typedef sinsp_curl::bearer_token::ptr_t bt_ptr_t;

	static const int default_timeout_ms = 1000L;

	k8s_handler(const std::string& id,
		std::string url,
		const std::string& path,
		const std::string& state_filter,
		const std::string& event_filter,
		const std::string& http_version = "1.0",
		int timeout_ms = default_timeout_ms,
		ssl_ptr_t ssl = 0,
		bt_ptr_t bt = 0);

	~k8s_handler();

	bool is_alive() const;
	void set_event_json(json_ptr_t json, const std::string&);
	const std::string& get_id() const;

	void collect_data();
	void set_machine_id(const std::string& machine_id);
	const std::string& get_machine_id() const;

protected:
	typedef std::unordered_set<std::string>    ip_addr_list_t;

	virtual void handle_json(Json::Value&& root) = 0;
	static bool is_ip_address(const std::string& addr);

private:
	typedef void (k8s_handler::*callback_func_t)(json_ptr_t, const std::string&);

	typedef socket_data_handler<k8s_handler> handler_t;
	typedef handler_t::ptr_t                 handler_ptr_t;
	typedef socket_collector<handler_t>      collector_t;
	typedef std::vector<json_ptr_t>          event_list_t;

	static ip_addr_list_t hostname_to_ip(const std::string& hostname);

	void make_http();
	void send_data_request();
	void check_collector_status(int expected);

	bool connect(int expected_connections);

	const std::string& translate_name(const std::string& event_name);

	handler_ptr_t  m_http;
	std::string    m_id;
	std::string    m_path;
	std::string    m_state_filter;
	std::string    m_event_filter;
	std::string&   m_filter;
	collector_t    m_collector;
	std::string    m_event_uri;
	event_list_t   m_events;
	long           m_timeout_ms;
	std::string    m_machine_id;
	json_query     m_jq;
	std::string    m_url;
	std::string    m_http_version;
	ssl_ptr_t      m_ssl;
	bt_ptr_t       m_bt;
	bool           m_req_sent = false;
	bool           m_state_built = false;
};

inline const std::string& k8s_handler::get_id() const
{
	return m_id;
}

inline void k8s_handler::set_machine_id(const std::string& machine_id)
{
	m_machine_id = machine_id;
}

inline const std::string& k8s_handler::get_machine_id() const
{
	return m_machine_id;
}
