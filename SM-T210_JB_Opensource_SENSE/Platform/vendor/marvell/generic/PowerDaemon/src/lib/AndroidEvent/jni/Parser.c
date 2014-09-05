#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <utils/Log.h>
#include <errno.h>

#include "Parser.h"

#define LEGACY_CONFIG_FILE_PATH "/etc/powerdaemon.xml"
#define CONFIG_FILE_PATH "/etc/powerdaemon_z3.xml"
#define MAX_NAME_LEN 12

static struct listnode* cmd_list = NULL;
extern int chip_rev;

struct node_info_t {
	xmlNodePtr pXmlNode;
	char parentNodeName[MAX_NAME_LEN];
};

struct xml_node_t {
	struct node_info_t nodeInfo;
	struct listnode link;
};

static void free_cmd_list(struct listnode*);
static void free_xmlNode_list(struct listnode*);

/*
 *  get the subsection of xml for name recursively, merge all the candidate.
 */
static struct listnode* get_subSection_by_name(xmlNodePtr root,char* name)
{
	struct listnode *list = (struct listnode *)calloc(1, sizeof(struct listnode));
	list_init(list);

	xmlNodePtr pCurNode = root->xmlChildrenNode;
	struct xml_node_t *n = NULL;

	while (pCurNode) {
		if (pCurNode->type != XML_ELEMENT_NODE) {
			pCurNode = pCurNode->next;
			continue;
		}

		if (strcmp((const char *)(pCurNode->name), name) == 0) { /* found */
			n = (struct xml_node_t *)calloc(1, sizeof(struct xml_node_t));
			n->nodeInfo.pXmlNode = pCurNode;
			strncpy(n->nodeInfo.parentNodeName, (char const *)root->name, MAX_NAME_LEN);
			list_add_tail(list, &(n->link));
			pCurNode = pCurNode->next;
			continue;
		}

		/* not found, search in the sub section */
		struct listnode *pTargetNodeList = get_subSection_by_name(pCurNode,name);

		if (pTargetNodeList != NULL) {
			/* find in the sub tree, merge!!!*/
			struct listnode *node = pTargetNodeList->next;
			for (; node != pTargetNodeList; node = pTargetNodeList->next) {
				struct xml_node_t* subNode = (struct xml_node_t*)node_to_item(node, struct xml_node_t, link);
				list_remove(node);
				list_add_tail(list, &(subNode->link));
			}
			free(pTargetNodeList);
		}

		pCurNode = pCurNode->next;
	}

	if (list_empty(list)) {
		free(list);
		return NULL;
	}

	return list;
}

/*
 *  get the pluging tag from its name
 */
static int get_plugin_tag_from_name(char *name) {
	int i = 0;
	while (plugin_infos[i].name != NULL) {
		if (strcmp(name, plugin_infos[i].name) == 0) {
			return plugin_infos[i].tags;
		}
		i++;
	}
	return -1;
}

/*
 *  fill the event data by event_type and arg
 *  @Param ev: the event
 *  @Param keyword: the keyword of event
 *  @Param cmd: the cmd that the event points to
 *  @Param arg: extra value for CHAR_EVENT / status for STATUS_EVENT
 *  @Param event_type: the type of the event,  STATUS_EVENT/CHAR_EVENT
 */
static void fill_event(Event *ev, char *keyword, struct command_t*cmd, char *arg, int event_type)
{
	ev->event_type = event_type;
	ev->cmd = (Command *)cmd;
	if (arg != NULL) {
		if (event_type == STATUS_EVENT) {
			ev->ev_data.state = atoi(arg);
		} else if (event_type == CHAR_EVENT) {
			memcpy(ev->ev_data.extra, arg, strlen(arg));
		} else {
			LOGW("Unhandled event type. New type %d?\n", event_type);
		}
	} else {
		/* NOTICE: case like monitor task */
		LOGW("Hey, extra without value, is it legeal?--keyword %s\n", keyword);
	}
}

/*
 *  gen the command of specific event for specfic plugin
 *  @Param ev: the event
 *  @Param keyword: the keyword of event
 *  @Param arg: extra value for CHAR_EVENT / status for STATUS_EVENT
 *  @Param event_type: the type of the event,  STATUS_EVENT/CHAR_EVENT
 *  @Param plugin: the name of plugin
 */
static void gen_command_for_event(Event *ev, char *keyword, char *arg, int event_type, char *plugin)
{
	struct command_t* cmd = NULL;
	struct listnode *node = NULL;

	int plugin_tag = get_plugin_tag_from_name(plugin);

	/* search the cmd list for the specific keyword */
	list_for_each(node, cmd_list) {
		cmd = (struct command_t*)node_to_item(node, Command, link);
		if (cmd->tag == plugin_tag) { //found!!!
			if (plugin_tag == TAG_ANDROID) {
				//NOTICE: Right now, Android plugin has many keywords.
				if (strcmp(cmd->plugin_data.android_data.keyword, keyword) == 0) {
					fill_event(ev, keyword, cmd, arg, event_type);
					return;
				}
			} else if (plugin_tag == TAG_THERMAL || plugin_tag == TAG_DDR_HOTPLUG) {
				//FIXME: Right now, these plugin has only one keyword, if found, fill it.
				fill_event(ev, keyword, cmd, arg, event_type);
				return;
			} else {
				//TO DO: more plugins
				LOGW("Hey, unhandle plugin event %s in Cmd list, is it legeal?\n", plugin);
			}
		}
    }

	/* Not found in cmd list, create a new command. */
	Command *new_cmd = (Command *)calloc(1, sizeof(Command));
	new_cmd->cmd.tag = get_plugin_tag_from_name(plugin);

	if (new_cmd->cmd.tag == TAG_ANDROID) {
		new_cmd->cmd.len = sizeof(struct android_event);
		memcpy(new_cmd->cmd.plugin_data.android_data.keyword, keyword, strlen(keyword));
	} else if (new_cmd->cmd.tag == TAG_THERMAL) {
		new_cmd->cmd.len = sizeof(struct thermal_info); /* only concern len*/
	} else if (new_cmd->cmd.tag == TAG_DDR_HOTPLUG) {
		new_cmd->cmd.len = sizeof(struct ddr_hotplug_info); /* only concern len*/
	} else {
		//TO DO: more plugins
		LOGW("Hey, unhandle plugin event %s when added, is it legeal?\n", plugin);
	}

	/* add command */
	list_add_tail(cmd_list, &(new_cmd->link));

	/* init event */
	fill_event(ev, keyword, new_cmd, arg, event_type);
}

/*
 *  gen the command of specific event for specfic plugin
 */
static void gen_event_extra_type(struct extra_t *e,char *type)
{
	if (strcmp(type,"String") == 0) {
		e->extra_type = EXTRA_STRINGS;
	} else if (strcmp(type,"int") == 0) {
		e->extra_type = EXTRA_INT;
	} else if (strcmp(type,"long") == 0) {
		e->extra_type = EXTRA_LONG;
	} else if (strcmp(type,"Boolean") == 0) {
		e->extra_type = EXTRA_BOOLEAN;
	} else
		LOGW("Unknown event type!");
}

/*DUMP*/
void dump_config(struct listnode *list)
{
	Event* ev = NULL;
	LOGD("==================================================================================");
	LOGD("Event: Keyword:name, Type: STATUS_EVENT/CHAR_EVENT, Status: 0/1, Extra: <type:name:value>");
	LOGD("==================================================================================");
	struct listnode *node = NULL;
	list_for_each(node, list) {
		Event *ev = node_to_item(node, Event, link);
		struct command_t* cmd = (struct command_t*)ev->cmd;
		LOGD("Event: ");
		if (ev->event_type == STATUS_EVENT) {
			if (cmd->tag == TAG_ANDROID) {
				LOGD("Keyword: %12s, Type: %12s, Status: %d, Extra: <%2d:%s:%s>",
					cmd->plugin_data.android_data.keyword, "STATUS_EVENT",
					ev->ev_data.state, ev->extra_info ? ev->extra_info->extra_type : -1,
					ev->extra_info ? ev->extra_info->extra_name : "none",
					ev->extra_info ? ev->extra_info->extra_value : "none");
				LOGD("Command: %p, Tag: %d, len: %d, keyword: %s",
					cmd, cmd->tag, cmd->len, cmd->plugin_data.android_data.keyword);
			} else if (cmd->tag == TAG_THERMAL) {
				LOGD("Keyword: %12s, Type: %12s, Status: %d, Extra: <%2d:%s:%s>",
					"thremal", "STATUS_EVENT", ev->ev_data.state,
					ev->extra_info ? ev->extra_info->extra_type : -1,
					ev->extra_info ? ev->extra_info->extra_name : "none",
					ev->extra_info ? ev->extra_info->extra_value : "none");
				LOGD("Command: %p, Tag: %d, len: %d, state: %d",
					cmd, cmd->tag, cmd->len, cmd->plugin_data.thermal_data.state);
			} else if (cmd->tag == TAG_DDR_HOTPLUG) {
				LOGD("Keyword: %12s, Type: %12s, Status: %d, Extra: <%2d:%s:%s>",
					"ddrhotplug", "STATUS_EVENT", ev->ev_data.state,
					ev->extra_info ? ev->extra_info->extra_type : -1,
					ev->extra_info ? ev->extra_info->extra_name : "none",
					ev->extra_info ? ev->extra_info->extra_value : "none");
				LOGD("Command: %p, Tag: %d, len: %d, state: %d",
					cmd, cmd->tag, cmd->len, cmd->plugin_data.ddr_hotplug_data.state);
			} else {
				LOGD("Other plugin %d", cmd->tag);
			}
		} else if (ev->event_type == CHAR_EVENT) {
			if (cmd->tag == TAG_ANDROID) {
				LOGD("Keyword: %12s, Type: %12s, Extra: <%2d:%s:%s>",
					cmd->plugin_data.android_data.keyword, "CHAR_EVENT",
					ev->extra_info ? ev->extra_info->extra_type : -1,
					ev->extra_info ? ev->extra_info->extra_name : "none",
					ev->extra_info ? ev->extra_info->extra_value : "none");
				LOGD("Command: %p, Tag: %d, len: %d, keyword: %s",
					cmd, cmd->tag, cmd->len, cmd->plugin_data.android_data.keyword);
			} else {
				LOGD("Other plugin %d", cmd->tag);
			}
		} else {
			LOGD("Unknown event type");
		}
	}
}

static void free_xmlNode_list(struct listnode* list)
{
	struct listnode *node = NULL;
	struct listnode *temp = NULL;
	for (node=list->next; node !=(list); ) {
		struct xml_node_t *pXmlNodePtr = node_to_item(node, struct xml_node_t, link);
		temp = node->next;
		list_remove(node);
		free(pXmlNodePtr);
		node = temp;
	}
	free(list);
}

static void free_cmd_list(struct listnode* list)
{
	struct listnode *node = NULL;
	struct listnode *temp = NULL;
	for (node=list->next; node !=(list); ) {
		Command *pXmlNodePtr = node_to_item(node, Command, link);
		temp = node->next;
		list_remove(node);
		free(pXmlNodePtr);
		node = temp;
	}
	free(list);
}

void free_config_list(struct listnode* list)
{
	free_cmd_list(cmd_list);
	struct listnode *node = NULL;
	struct listnode *temp = NULL;
	for (node=list->next; node !=(list); ) {
		Event *pXmlNodePtr = node_to_item(node, Event, link);
		temp = node->next;
		list_remove(node);
		free(pXmlNodePtr);
		node = temp;
	}
	free(list);
}

/*
  *  init all the events
  */
int init_event_config(struct listnode* event_list)
{
	if (event_list == NULL) {
		LOGE("Create the event config list failed!!!");
		return -1;
	}

	/* init the cmd list */
	if (cmd_list == NULL) {
		cmd_list = (struct listnode*)calloc(1,sizeof(struct listnode*));
	}
	list_init(cmd_list);

	/* chip revision */
	xmlDocPtr doc = NULL;
	if(chip_rev >= PXA988_Z3) {
		LOGD("It's above Z3 revision, reading configuration file %s", CONFIG_FILE_PATH);
		doc = xmlParseFile(CONFIG_FILE_PATH);
	} else {
		LOGD("It's under Z3 revision, reading configuration file %s", LEGACY_CONFIG_FILE_PATH);
		doc = xmlParseFile(LEGACY_CONFIG_FILE_PATH);
	}

	if (doc == NULL) {
		LOGE("failed to parse xml file");
		return -1;
	}

    /* start xml parse */
    xmlNodePtr pRoot = xmlDocGetRootElement(doc);
    if (pRoot == NULL) {
        LOGE("failed to get root element");
        return -1;
    }

    if (xmlStrcmp(pRoot->name, (const xmlChar *)"MarvellPowerPolicyDaemon")) {
        LOGE("wrong config file, root node != MarvellPowerPolicyDaemon");
        xmlFreeDoc(doc);
        return -1;
    }

    struct listnode* pEventNodeList = get_subSection_by_name(pRoot, "event");
    if (pEventNodeList == NULL) {
        LOGE("wrong config file, No event????");
        xmlFreeDoc(doc);
        return -1;
    }

    xmlChar *event_keyword,*event_intent_name,*event_default_status;
    Event* ev = NULL;

    /*parse event section */
	struct listnode *node = NULL;
	list_for_each(node, pEventNodeList) {
        struct xml_node_t *pXmlNodePtr = node_to_item(node, struct xml_node_t, link);
        xmlNodePtr pEventNode = pXmlNodePtr->nodeInfo.pXmlNode;
        event_keyword = xmlGetProp(pEventNode, (const xmlChar *)"keyword");
        event_intent_name = xmlGetProp(pEventNode, (const xmlChar *)"intent");
        event_default_status = xmlGetProp(pEventNode, (const xmlChar *)"status");

		char *plugin = pXmlNodePtr->nodeInfo.parentNodeName;

		//LOGD("Get Event: keyword %s,intent_name %s, staus: %s, plugin: %s\n",
		//	event_keyword, event_intent_name, event_default_status, plugin);

        /* Now parse the sub extra section if needed */
        struct listnode *pExtraNodeList = get_subSection_by_name(pEventNode, "extra");

		/* event without extra */
		if (pExtraNodeList == NULL) {
			/* NOTICE: event without status attribute must have extra sections as args */
            if (event_default_status == NULL) {
                LOGE("Error config file, need default status for event without extra!");
                free_xmlNode_list(pEventNodeList);
                xmlFreeDoc(doc);
                return -1;
            }

			/* init and add the event */
			ev = (Event *)calloc(1, sizeof(Event));
			memcpy(ev->intent_info, event_intent_name, strlen((char *)event_intent_name));

			gen_command_for_event(ev, (char *)event_keyword,
				(char *)event_default_status, STATUS_EVENT, plugin);

            list_add_tail(event_list,&(ev->link));
        } else {  /* event with extra */
            xmlChar *extra_type,*extra_name,*extra_value,*status;
			struct listnode *extra_list_node = NULL;
            list_for_each(extra_list_node, pExtraNodeList) {
				struct xml_node_t *pXmlNodePtr = node_to_item(extra_list_node, struct xml_node_t, link);
				xmlNodePtr pExtraNode = pXmlNodePtr->nodeInfo.pXmlNode;

                extra_type = xmlGetProp(pExtraNode, (const xmlChar *)"type");
                extra_name = xmlGetProp(pExtraNode, (const xmlChar *)"name");
                extra_value = xmlGetProp(pExtraNode, (const xmlChar *)"value");
                status = xmlGetProp(pExtraNode, (const xmlChar *)"status");

				//LOGD("Get extra: name %s,type %s, value %s, staus: %s",
				//	extra_name,extra_type,extra_value,status);

				/* Init the extra section */
				/* NOTICE: Case like monitor task has no value for the extra in the config */
                struct extra_t * ex = (struct extra_t *)calloc(1,sizeof(struct extra_t));
                memcpy(ex->extra_name, (char *)extra_name, strlen((char *)extra_name)); /* name */
                gen_event_extra_type(ex, (char *)extra_type); /* type */
                if (extra_value != NULL) {
					memcpy(ex->extra_value, (char *)extra_value, strlen((char *)extra_value)); /* value */
				}

				/* init and add event */
				ev = (Event *)calloc(1,sizeof(Event));
				memcpy(ev->intent_info, event_intent_name, strlen((char *)event_intent_name));
                ev->extra_info = ex;

                if (status != NULL) { /* status event with extra */
                    gen_command_for_event(ev, (char *)event_keyword,
						(char *)status, STATUS_EVENT, plugin);
                } else { /* char event with extra */
                    gen_command_for_event(ev, (char *)event_keyword,
						(char *)extra_value, CHAR_EVENT, plugin);
                }
                list_add_tail(event_list,&(ev->link));
            }
            free_xmlNode_list(pExtraNodeList);
        }
    }
    free_xmlNode_list(pEventNodeList);

    dump_config(event_list);
    return 0;
}
