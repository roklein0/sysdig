//
// k8s_net.h
//
// connects and gets the data from k8s_net REST API interface
//
#pragma once

#ifdef HAS_CAPTURE

#include "k8s_component.h"
#include "k8s_event_data.h"
//#include "k8s_http.h"
//#include "k8s_collector.h"
#include "k8s_handler.h"
#include "k8s_event_data.h"
#include "uri.h"
#include "sinsp_curl.h"
#include <sstream>
#include <utility>

class k8s;

class k8s_net
{
public:
	typedef sinsp_curl::ssl::ptr_t          ssl_ptr_t;
	typedef sinsp_curl::bearer_token::ptr_t bt_ptr_t;
	typedef k8s_component::ext_list_ptr_t   ext_list_ptr_t;
	typedef user_event_filter_t::ptr_t     filter_ptr_t;

	k8s_net(k8s& kube, k8s_state_t& state, const std::string& uri = "http://localhost:80",
		ssl_ptr_t ssl = nullptr,
		bt_ptr_t bt = nullptr,
		bool curl_debug = false,
		ext_list_ptr_t extensions = nullptr,
		filter_ptr_t event_filter = nullptr);

	~k8s_net();

	//void get_all_data(const k8s_component::type_map::value_type& component, std::ostream& out);

	void add_handler(const k8s_component::type_map::value_type& component);
	void add_api_interface(const k8s_component::type_map::value_type& component);

	void watch();

	void stop_watching();

	bool is_healthy() const;

private:
	//void subscribe();

	//void unsubscribe();

	//void dispatch_events();

	void init();

	void end_thread();

	bool is_secure();

	void cleanup();

	//typedef std::map<k8s_component::type, k8s_http*> api_map_t;
	typedef k8s_handler::handler_t                       handler_t;
	typedef k8s_handler::ptr_t                           handler_ptr_t;
	typedef k8s_handler::collector_t                     collector_t;
	typedef std::map<k8s_component::type, handler_ptr_t> handler_map_t;

	k8s&           m_k8s;
	k8s_state_t&   m_state;
	collector_t    m_collector;
	uri            m_uri;
	ssl_ptr_t      m_ssl;
	bt_ptr_t       m_bt;
	bool           m_stopped;
	handler_map_t  m_handlers;
	//api_map_t      m_api_interfaces;
	//k8s_collector  m_collector;
	bool           m_curl_debug;
	ext_list_ptr_t m_extensions;
	filter_ptr_t   m_event_filter;
#ifndef K8S_DISABLE_THREAD
	std::thread*   m_thread;
#endif
};

inline bool k8s_net::is_secure()
{
	return m_uri.get_scheme() == "https";
}

inline bool k8s_net::is_healthy() const
{
	return true;/*m_collector.subscription_count() ==
		static_cast<int>(m_api_interfaces.size());*/
}

#endif // HAS_CAPTURE

