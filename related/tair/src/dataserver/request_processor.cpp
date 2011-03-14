/*
 * (C) 2007-2010 Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * request dispatcher impl
 *
 * Version: $Id: request_processor.cpp 28 2010-09-19 05:18:09Z ruohai@taobao.com $
 *
 * Authors:
 *   ruohai <ruohai@taobao.com>
 *     - initial release
 *
 */
#include "request_processor.hpp"

namespace tair {
   request_processor::request_processor(tair_manager *tair_mgr, heartbeat_thread *heart_beat, tbnet::ConnectionManager *connection_mgr)
   {
      this->tair_mgr = tair_mgr;
      this->heart_beat = heart_beat;
      this->connection_mgr = connection_mgr;
   }

   request_processor::~request_processor()
   {
   }

   int request_processor::process(request_put *request, bool &send_return)
   {
      int rc = 0;

      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      if (request->key.get_size() == 0 || request->data.get_size() == 0) {
         log_debug("put value is null, return failed");
         return TAIR_RETURN_ITEMSIZE_ERROR;
      }

      uint64_t target_server_id = 0;
      if (tair_mgr->should_proxy(request->key, target_server_id))
      {
         base_packet *proxy_packet = new request_put(*(request_put*)request);
         rc = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
         return rc;
      }

      request->key.server_flag = request->server_flag;
      request->key.data_meta.version = request->version;
      log_debug("receive put request, serverFlag: %d", request->key.server_flag);
      PROFILER_START("put operation start");
      plugin::plugins_root* plugin_root = NULL;
      PROFILER_BEGIN("do request plugin");
      int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                    TAIR_REQ_PUT_PACKET, request->area, &(request->key), &(request->data), plugin_root);
      PROFILER_END();
      send_return = true;
      if (plugin_ret < 0) {
         log_debug("plugin return %d, skip excute", plugin_ret);
         rc = TAIR_RETURN_PLUGIN_ERROR;
      }
      else {
         PROFILER_BEGIN("do put");
         rc = tair_mgr->put(request->area, request->key, request->data, request->expired);
         PROFILER_END();

         PROFILER_BEGIN("do response plugin");
         tair_mgr->plugins_manager.do_response_plugins(rc, plugin::PLUGIN_TYPE_SYSTEM,
                                                       TAIR_REQ_PUT_PACKET, request->area, &(request->key), &(request->data), plugin_root);
         PROFILER_END();
      }
      log_debug("put request return: %d", rc);
      PROFILER_DUMP();
      PROFILER_STOP();

      return rc;
   }

   int request_processor::process(request_get *request, bool &send_return)
   {
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      set<data_entry*, data_entry_comparator>::iterator it;
      data_entry *data = NULL;
      response_get *resp = new response_get();

      uint64_t target_server_id = 0;
      int rc = TAIR_RETURN_FAILED;

      if (request->key_list != NULL) {
         uint32_t count = 0;
         PROFILER_START("batch get operation start");
         for (it = request->key_list->begin(); it != request->key_list->end(); ++it) {
            PROFILER_BEGIN("sub get operation in batch begin");
            if (data == NULL) {
               data = new data_entry();
            }
            data_entry *key = (*it);

            if (tair_mgr->should_proxy(*key, target_server_id))
            {
               resp->add_proxyed_key(key);
               continue;
            }

            plugin::plugins_root* plugin_root = NULL;
            PROFILER_BEGIN("do request plugin");
            int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                          TAIR_REQ_GET_PACKET, request->area, key, NULL, plugin_root);
            PROFILER_END();
            if (plugin_ret < 0) {
               log_debug("plugin return %d, skip excute", plugin_ret);
               continue;
            }
            PROFILER_BEGIN("do get");
            int rev = tair_mgr->get(request->area, *key, *data);
            PROFILER_END();

            PROFILER_BEGIN("do response plugin");
            tair_mgr->plugins_manager.do_response_plugins(rev, plugin::PLUGIN_TYPE_SYSTEM,
                                                          TAIR_REQ_GET_PACKET, request->area, key, data, plugin_root);
            PROFILER_END();
            if (rev != TAIR_RETURN_SUCCESS) {
               continue;
            }
            ++count;
            resp->add_key_data(new data_entry(*key), data);
            data = NULL;
            PROFILER_END();
         }
         if (data != NULL) {
            delete data;
            data = NULL;
         }
         if(count == request->key_count){
            rc = TAIR_RETURN_SUCCESS;
         }else if(count > 0){
            rc = TAIR_RETURN_PARTIAL_SUCCESS;
         }else {
            rc = TAIR_RETURN_FAILED;
         }
      } else if (request->key != NULL) {
         PROFILER_START("get operation start");

         if (tair_mgr->should_proxy(*request->key, target_server_id))
         {
            base_packet *proxy_packet = new request_get(*(request_get*)request);
            rc = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
            delete resp; resp = 0;
            return rc;
         }

         plugin::plugins_root* plugin_root = NULL;
         PROFILER_BEGIN("do request plugin");
         int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                       TAIR_REQ_GET_PACKET,request->area, (request->key), NULL, plugin_root);
         PROFILER_END();

         if (plugin_ret < 0) {
            log_debug("plugin return %d, skip excute", plugin_ret);
            rc = TAIR_RETURN_PLUGIN_ERROR;
         }
         else {
            data = new data_entry();
            PROFILER_BEGIN("do get");
            rc = tair_mgr->get(request->area, *(request->key), *data);
            PROFILER_END();

            PROFILER_BEGIN("do response plugin");
            tair_mgr->plugins_manager.do_response_plugins(rc, plugin::PLUGIN_TYPE_SYSTEM,
                                                          TAIR_REQ_GET_PACKET,request->area, (request->key), data, plugin_root);
            PROFILER_END();
            if (rc == TAIR_RETURN_SUCCESS) {
               resp->add_key_data(request->key, data);
               request->key = NULL;
               log_debug("get return %s", resp->data->get_data());
            } else {
               delete data;
            }
         }
      }
      resp->config_version = heart_beat->get_client_version();
      resp->setChannelId(request->getChannelId());
      resp->set_code(rc);
      if(request->get_connection()->postPacket(resp) == false) {
         delete resp;
         resp = 0;
      }
      PROFILER_DUMP();
      PROFILER_STOP();

      send_return = false;
      return rc;
   }

   int request_processor::process(request_remove *request, bool &send_return)
   {
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      int rc = 0;
      set<data_entry*, data_entry_comparator>::iterator it;
      uint32_t count = 0;
      plugin::plugins_root* plugin_root = NULL;
      uint64_t target_server_id = 0;
      send_return = true;

      if (request->key_list != NULL) {
         PROFILER_START("batch remove operation start");
         for (it = request->key_list->begin(); it != request->key_list->end(); ++it) {
            PROFILER_BEGIN("sub remove operation in batch begin");
            data_entry *key = (*it);
            key->server_flag = request->server_flag;

            if (tair_mgr->should_proxy(*key, target_server_id))
            {
               // proxyed, we will not touch it
               continue;
            }

            PROFILER_BEGIN("do request plugin");
            int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                          TAIR_REQ_REMOVE_PACKET, request->area, key, NULL, plugin_root);
            PROFILER_END();
            if (plugin_ret < 0) {
               log_debug("plugin return %d, skip excute", plugin_ret);
               continue;
            }
            PROFILER_BEGIN("do remove");
            int rev = tair_mgr->remove(request->area, *key);
            PROFILER_END();

            PROFILER_BEGIN("do response plugin");
            tair_mgr->plugins_manager.do_response_plugins(rev, plugin::PLUGIN_TYPE_SYSTEM,
                                                          TAIR_REQ_REMOVE_PACKET, request->area, key, NULL, plugin_root);
            PROFILER_END();
            if (rev == TAIR_RETURN_SUCCESS) {
               count ++;
            }
            PROFILER_END();
         }
         if (count != request->key_list->size()) rc = EXIT_FAILURE;
      } else if (request->key != NULL) {
         PROFILER_START("remove operation start");
         request->key->server_flag = request->server_flag;

         if (tair_mgr->should_proxy(*request->key, target_server_id))
         {
            base_packet *proxy_packet = new request_remove(*(request_remove*)request);
            rc = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
            return rc;
         }

         PROFILER_BEGIN("do request plugin");
         int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                       TAIR_REQ_REMOVE_PACKET, request->area, request->key, NULL, plugin_root);
         PROFILER_END();
         if (plugin_ret < 0) {
            log_debug("plugin return %d, skip excute", plugin_ret);
            rc = TAIR_RETURN_PLUGIN_ERROR;
         }else {
            PROFILER_BEGIN("do remove");
            rc = tair_mgr->remove(request->area, *(request->key));
            PROFILER_END();

            PROFILER_BEGIN("do response plugin");
            tair_mgr->plugins_manager.do_response_plugins(rc, plugin::PLUGIN_TYPE_SYSTEM,
                                                          TAIR_REQ_REMOVE_PACKET, request->area, (request->key), NULL, plugin_root);
            PROFILER_END();
         }
         PROFILER_END();
      }
      PROFILER_DUMP();
      PROFILER_STOP();

      return rc;
   }

   int request_processor::process(request_inc_dec *request, bool &send_return)
   {
      int rc = 0;
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      uint64_t target_server_id = 0;
      if (tair_mgr->should_proxy(request->key, target_server_id))
      {
         base_packet *proxy_packet = new request_inc_dec(*(request_inc_dec*)request);
         rc = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
         return rc;
      }

      request->key.server_flag = request->server_flag;

      int ret_value = 0;
      PROFILER_START("addcounter operation start");
      plugin::plugins_root* plugin_root = NULL;
      PROFILER_BEGIN("do request plugin");
      int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                    TAIR_REQ_INCDEC_PACKET, request->area, &(request->key), NULL, plugin_root);
      PROFILER_END();
      if (plugin_ret < 0) {
         log_debug("plugin return %d, skip excute", plugin_ret);
         rc = TAIR_RETURN_PLUGIN_ERROR;
         send_return = true;
      } else {
         PROFILER_BEGIN("do addCount");
         rc = tair_mgr->add_count(request->area, request->key,
                                  request->add_count, request->init_value, &ret_value,request->expired);
         PROFILER_END();

         PROFILER_BEGIN("do response plugins");
         tair_mgr->plugins_manager.do_response_plugins(rc, plugin::PLUGIN_TYPE_SYSTEM,
                                                       TAIR_REQ_INCDEC_PACKET, request->area, &(request->key), NULL, plugin_root);
         PROFILER_END();

         if (rc != TAIR_RETURN_SUCCESS) {
            send_return = true;
         } else {
            response_inc_dec *resp = new response_inc_dec();
            resp->value = ret_value;
            resp->config_version = heart_beat->get_client_version();
            resp->setChannelId(request->getChannelId());
            if (request->get_connection()->postPacket(resp) == false) {
               delete resp;
            }
            send_return = false;
         }
      }
      PROFILER_DUMP();
      PROFILER_STOP();
      return rc;
   }

   int request_processor::process(request_duplicate *request, bool &sendReturn)
   {
      int ret = 0;

      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      PROFILER_START("start duplicate operation");
      log_debug("request key serverflag = %d", request->key.server_flag);
      if (request->data.data_meta.flag == TAIR_ITEM_FLAG_DELETED) {
         // remove
         log_debug("duplicate remove operate");
         request->key.data_meta.version = 0; // force remove
         PROFILER_BEGIN("do remove operation");
         ret = tair_mgr->remove(request->area, request->key);
         PROFILER_END();
      } else {
         // put
         log_debug("duplicate put operate");
         PROFILER_BEGIN("do put operation");
         ret = tair_mgr->put(request->area, request->key, request->data, request->key.data_meta.edate);
         PROFILER_END();
      }
      log_debug("duplicate return code: %d", ret);
      if (ret == TAIR_RETURN_SUCCESS || ret == TAIR_RETURN_DATA_NOT_EXIST) {
         log_debug("send return packet to duplicate source");
         response_duplicate *dresp = new response_duplicate();
         dresp->setChannelId(request->getChannelId());
         dresp->server_id = local_server_ip::ip;
         dresp->packet_id = request->packet_id;
         dresp->bucket_id = request->bucket_number;
         if (request->get_connection()->postPacket(dresp) == false) {
            delete dresp;
         }
      } else {
         log_debug("XXX do duplicate operation failed");
      }
      PROFILER_DUMP();
      PROFILER_STOP();
      sendReturn = false;
      return ret;
   }

   int request_processor::process(request_add_items *request, bool &send_return)
   {
      int ret = 0;

      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      PROFILER_START("add items operation start");
      uint64_t target_server_id = 0;
      if (tair_mgr->should_proxy(request->key, target_server_id))
      {
         base_packet *proxy_packet = new request_add_items(*(request_add_items*)request);
         ret = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
         return ret;
      }

      request->key.server_flag = request->server_flag;
      request->key.data_meta.version = request->version;

      plugin::plugins_root* plugin_root = NULL;
      PROFILER_BEGIN("do request plugins");
      int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                    TAIR_REQ_ADDITEMS_PACKET, request->area, &(request->key), &(request->data), plugin_root);
      PROFILER_END();
      if (plugin_ret < 0) {
         log_debug("plugin return %d, skip excute", plugin_ret);
         ret = TAIR_RETURN_PLUGIN_ERROR;
      }else {
         PROFILER_BEGIN("do add items");
         ret = tair_mgr->add_items(request->area,request->key,request->data,request->max_count,request->expired);
         PROFILER_END();

         PROFILER_BEGIN("do response plugins");
         tair_mgr->plugins_manager.do_response_plugins(ret, plugin::PLUGIN_TYPE_SYSTEM,
                                                       TAIR_REQ_ADDITEMS_PACKET, request->area, &(request->key), &(request->data), plugin_root);
         PROFILER_END();

      }
      PROFILER_DUMP();
      PROFILER_STOP();
      send_return = true;
      return ret;
   }

   int request_processor::process(request_get_items *request, bool &send_return)
   {
      int ret = 0;
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      PROFILER_START("get items operation start");
      uint64_t target_server_id = 0;
      if (tair_mgr->should_proxy(*(request->key), target_server_id))
      {
         base_packet *proxy_packet = new request_get_items(*(request_get_items*)request);
         ret = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
         return ret;
      }

      plugin::plugins_root* plugin_root = NULL;
      PROFILER_BEGIN("do request plugins");
      int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                    TAIR_REQ_GETITEMS_PACKET, request->area, (request->key), NULL, plugin_root);
      PROFILER_END();
      if (plugin_ret < 0) {
         log_debug("plugin return %d, skip excute", plugin_ret);
         ret = TAIR_RETURN_PLUGIN_ERROR;
         PROFILER_DUMP();
         PROFILER_STOP();
         return ret; //let caller respond
      }

      data_entry *data = new data_entry();
      response_get_items *resp = new response_get_items();
      PROFILER_BEGIN("do get items");
      ret = tair_mgr->get_items(request->area,*(request->key),request->offset,request->count,*data,request->type);
      PROFILER_END();

      PROFILER_BEGIN("do response plugins");
      tair_mgr->plugins_manager.do_response_plugins(ret, plugin::PLUGIN_TYPE_SYSTEM,
                                                    TAIR_REQ_GETITEMS_PACKET,request->area, (request->key), data, plugin_root);
      PROFILER_END();

      resp->set_code(ret);

      if( ret == TAIR_RETURN_SUCCESS){
         log_debug("getItems success,data:%s",data->get_data()+sizeof(uint32_t));
         resp->add_key_data(request->key, data);
         request->key = 0;
      }else{
         delete data;
         data = 0;
      }
      resp->config_version = heart_beat->get_client_version();
      resp->setChannelId(request->getChannelId());
      if(request->get_connection()->postPacket(resp) == false) {
         delete resp;
      }
      send_return = false;
      PROFILER_DUMP();
      PROFILER_STOP();
      return ret;
   }

   int request_processor::process(request_remove_items *request, bool &send_return)
   {
      int ret = 0;
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      uint64_t target_server_id = 0;

      PROFILER_START("remove items operation start");
      if (tair_mgr->should_proxy(*(request->key), target_server_id))
      {
         base_packet *proxy_packet = new request_remove_items(*(request_remove_items*)request);
         ret = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
         return ret;
      }

      request->key->server_flag = request->server_flag;

      plugin::plugins_root* plugin_root = NULL;
      PROFILER_BEGIN("do request plugins");
      int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                    TAIR_REQ_REMOVEITEMS_PACKET, request->area, (request->key), NULL, plugin_root);
      PROFILER_END();
      if (plugin_ret < 0) {
         log_debug("plugin return %d, skip excute", plugin_ret);
         ret = TAIR_RETURN_PLUGIN_ERROR;
      }else {
         PROFILER_BEGIN("do remove items");
         ret = tair_mgr->remove_items(request->area,*(request->key),request->offset,request->count);
         PROFILER_END();

         PROFILER_BEGIN("do response plugins");
         tair_mgr->plugins_manager.do_response_plugins(ret, plugin::PLUGIN_TYPE_SYSTEM,
                                                       TAIR_REQ_REMOVEITEMS_PACKET, request->area, (request->key), NULL, plugin_root);
         PROFILER_END();

      }
      PROFILER_DUMP();
      PROFILER_STOP();
      send_return = true;
      return ret;
   }

   int request_processor::process(request_get_and_remove_items *request, bool &send_return) 
   {
      int ret = 0;
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      PROFILER_START("getAndRemove items start");
      uint64_t target_server_id = 0;
      if (tair_mgr->should_proxy(*(request->key), target_server_id))
      {
         base_packet *proxy_packet = new request_get_and_remove_items(*(request_get_and_remove_items*)request);
         ret = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
         return ret;
      }

      request->key->server_flag = request->server_flag;

      plugin::plugins_root* plugin_root = NULL;
      PROFILER_BEGIN("do response plugins");
      int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,
                                                                    TAIR_REQ_GETANDREMOVEITEMS_PACKET, request->area, (request->key), NULL, plugin_root);
      PROFILER_END();
      if (plugin_ret < 0) {
         log_debug("plugin return %d, skip excute", plugin_ret);
         ret = TAIR_RETURN_PLUGIN_ERROR;
         return ret; //let caller respond
      }

      data_entry *data = new data_entry();
      response_get_items *resp = new response_get_items();
      PROFILER_BEGIN("do getAndRemove");
      ret = tair_mgr->get_and_remove(request->area,*(request->key),request->offset,request->count,*data,request->type);
      PROFILER_END();

      PROFILER_BEGIN("do response plugins");
      tair_mgr->plugins_manager.do_response_plugins(ret, plugin::PLUGIN_TYPE_SYSTEM,
                                                    TAIR_REQ_GETANDREMOVEITEMS_PACKET,request->area, (request->key), data, plugin_root);
      PROFILER_END();

      resp->set_code(ret);
      if(ret == TAIR_RETURN_SUCCESS){
         resp->add_key_data(request->key, data);
         request->key = 0;
      }else{
         delete data;
         data = 0;
      }
      resp->config_version = heart_beat->get_client_version();
      resp->setChannelId(request->getChannelId());
      if(request->get_connection()->postPacket(resp) == false) {
         delete resp;
      }
      PROFILER_DUMP();
      PROFILER_STOP();
      send_return = false;
      return ret;
   }

   int request_processor::process(request_get_items_count *request,bool& send_return)
   {
      int ret = 0;
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }
      uint64_t target_server_id = 0;
      PROFILER_START("getitems operation start");
      if (tair_mgr->should_proxy(*(request->key), target_server_id))
      {
         base_packet *proxy_packet = new request_get_items(*(request_get_items*)request);
         ret = do_proxy(target_server_id, proxy_packet, request) ? TAIR_RETURN_PROXYED : TAIR_RETURN_FAILED;
         return ret;
      }

      request->key->server_flag = request->server_flag;

      plugin::plugins_root* plugin_root = NULL;
      PROFILER_BEGIN("do request plugin");
      int plugin_ret = tair_mgr->plugins_manager.do_request_plugins(plugin::PLUGIN_TYPE_SYSTEM,TAIR_REQ_GETITEMSCOUNT_PACKET, request->area, (request->key), NULL, plugin_root);
      PROFILER_END();
      if (plugin_ret < 0) {
         log_debug("plugin return %d, skip excute", plugin_ret);
         ret = TAIR_RETURN_PLUGIN_ERROR;
      }else {

         PROFILER_BEGIN("do getItemCount");
         ret = tair_mgr->get_item_count(request->area,*(request->key));
         PROFILER_END();

         PROFILER_BEGIN("do response plugins");
         tair_mgr->plugins_manager.do_response_plugins(ret, plugin::PLUGIN_TYPE_SYSTEM, TAIR_REQ_GETITEMSCOUNT_PACKET,request->area, (request->key), NULL, plugin_root);
         PROFILER_END();
      }
      send_return = true;
      PROFILER_DUMP();
      PROFILER_STOP();
      return ret;

   }

   int request_processor::process(request_mupdate *request,bool &send_return)
   {
      if (tair_mgr->is_working() == false) {
         return TAIR_RETURN_SERVER_CAN_NOT_WORK;
      }

      tair_operc_vector::iterator it;

      if (request->server_flag != TAIR_SERVERFLAG_MIGRATE) {
         log_warn("requestMUpdatePacket have no MIGRATE flag");
         return TAIR_RETURN_INVALID_ARGUMENT;
      }

      int rc = TAIR_RETURN_SUCCESS;
      if (request->key_and_values != NULL) {
         for (it = request->key_and_values->begin(); it != request->key_and_values->end(); ++it) {
            operation_record *oprec = (*it);
            if (oprec->operation_type == 1) {
               // put
               rc = tair_mgr->direct_put(*oprec->key, *oprec->value);
            } else if (oprec->operation_type == 2) {
               // remove
               rc = tair_mgr->direct_remove(*oprec->key);
            } else {
               rc = TAIR_RETURN_INVALID_ARGUMENT;
               break;
            }

            if (rc != TAIR_RETURN_SUCCESS) {
               log_debug("do migrate operation failed, rc: %d", rc);
               break;
            }
         }
      }

      return rc;
   }

   bool request_processor::do_proxy(uint64_t target_server_id, base_packet *proxy_packet, base_packet *packet)
   {
      proxy_packet->server_flag = TAIR_SERVERFLAG_PROXY;
      bool rc = connection_mgr->sendPacket(target_server_id, proxy_packet, NULL, packet);
      if (rc == false) {
         delete proxy_packet;
      }
      return rc;
   }


}
