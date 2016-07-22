//
// k8s_handler.h
//

#pragma once

#include "json/json.h"
#include "socket_collector.h"
#include "k8s_state.h"
#include <unordered_set>

class sinsp;

class k8s_handler
{
public:
	enum msg_reason
	{
		COMPONENT_ADDED,
		COMPONENT_MODIFIED,
		COMPONENT_DELETED,
		COMPONENT_ERROR,
		COMPONENT_UNKNOWN // only to mark bad event messages
	};

	struct msg_data
	{
		msg_reason  m_reason = COMPONENT_UNKNOWN;
		std::string m_name;
		std::string m_uid;
		std::string m_namespace;
		std::string m_kind;

		bool is_valid() const
		{
			return m_reason != COMPONENT_UNKNOWN;
		}

		std::string get_reason_desc() const
		{
			switch(m_reason)
			{
				case COMPONENT_ADDED:    return "ADDED";
				case COMPONENT_MODIFIED: return "ADDED";
				case COMPONENT_DELETED:  return "ADDED";
				case COMPONENT_ERROR:    return "ADDED";
				case COMPONENT_UNKNOWN:
				default:                 return "UNKNOWN";
			}
			return "UNKNOWN";
		}
	};

	typedef std::shared_ptr<k8s_handler>     ptr_t;
	typedef std::vector<std::string>         uri_list_t;
	typedef std::shared_ptr<Json::Value>     json_ptr_t;
	typedef sinsp_curl::ssl::ptr_t           ssl_ptr_t;
	typedef sinsp_curl::bearer_token::ptr_t  bt_ptr_t;
	typedef socket_data_handler<k8s_handler> handler_t;
	typedef handler_t::ptr_t                 handler_ptr_t;
	typedef socket_collector<handler_t>      collector_t;

	static const int default_timeout_ms = 1000L;

	k8s_handler(collector_t& collector,
		const std::string& id,
		std::string url,
		const std::string& path,
		const std::string& state_filter,
		const std::string& event_filter,
		const std::string& http_version = "1.0",
		int timeout_ms = default_timeout_ms,
		ssl_ptr_t ssl = nullptr,
		bt_ptr_t bt = nullptr,
		k8s_state_t* state = nullptr);

	~k8s_handler();

	bool is_alive() const;
	void set_event_json(json_ptr_t json, const std::string&);
	const std::string& get_id() const;

	void collect_data();
	void set_machine_id(const std::string& machine_id);
	const std::string& get_machine_id() const;

	bool is_state_built() const;

protected:
	typedef std::unordered_set<std::string>    ip_addr_list_t;

	virtual void handle_json(Json::Value&& root);
	virtual void handle_component(const Json::Value& json, const msg_data* data = 0) = 0;
	msg_data get_msg_data(const std::string& evt, const std::string& type, const Json::Value& root);
	static bool is_ip_address(const std::string& addr);

	k8s_pair_list extract_object(const Json::Value& object);

	template <typename T>
	void handle_selectors(T& component, const Json::Value& selector)
	{
		if(!selector.isNull())
		{
			component.set_selectors(extract_object(selector));
		}
		else
		{
			g_logger.log("K8s Replication Controller: Null selector object.", sinsp_logger::SEV_ERROR);
		}
	}

	void log_event(const msg_data& data);
	void log_error(const Json::Value& root, const std::string& comp);

	k8s_state_t*        m_state = nullptr;

private:
	typedef void (k8s_handler::*callback_func_t)(json_ptr_t, const std::string&);

	typedef std::vector<json_ptr_t> event_list_t;

	static ip_addr_list_t hostname_to_ip(const std::string& hostname);

	void make_http();
	void send_data_request();
	void check_collector_status();

	bool connect();

	const std::string& translate_name(const std::string& event_name);

	collector_t&        m_collector;
	handler_ptr_t       m_http;
	std::string         m_id;
	std::string         m_path;
	std::string         m_state_filter;
	std::string         m_event_filter;
	std::string&        m_filter;
	std::string         m_event_uri;
	event_list_t        m_events;
	long                m_timeout_ms;
	std::string         m_machine_id;
	json_query          m_jq;
	std::string         m_url;
	std::string         m_http_version;
	ssl_ptr_t           m_ssl;
	bt_ptr_t            m_bt;
	bool                m_req_sent = false;
	bool                m_state_built = false;
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

inline bool k8s_handler::is_state_built() const
{
	return m_state_built;
}

inline void k8s_handler::log_event(const msg_data& data)
{
	g_logger.log("K8s " + data.get_reason_desc() + ' ' +
				 data.m_kind + ' ' +
				 data.m_name + " [" + data.m_uid + "]",
				 sinsp_logger::SEV_DEBUG);
}
