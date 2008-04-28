// preploc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\..\src\xmltools.h"
#include "arrays.h"

#define FREE_CLEAR_STR(str) if(str) delete str; str = NULL;
#define MAKE_STR(str, cpstr) FREE_CLEAR_STR(str); if(cpstr) { str = new WCHAR[lstrlen(cpstr) + 1]; lstrcpy(str, cpstr); }

struct LOC_STR
{
	LPTSTR			id;
	LPTSTR			attr;
	LPTSTR			defString;
	IXMLDOMNode*	node;
};

CTArray<LOC_STR>	gStrings;
struct
{
	LPTSTR	tag;
	LPTSTR	attr;
} tags[] = 
{
	{L"button",			L"label"		},
	{L"checkbox",		L"label"		},
	{L"checkitem",		L"label"		},
	{L"listitem",		L"label"		},
	{L"dialog",			L"caption"		},
	{L"ext",			L"text"			},
	{L"groupbox",		L"label"		},
	{L"label",			L"value"		},
	{L"listcol",		L"caption"		},
	{L"radio",			L"label"		},
	{L"selectfile",		L"title"		},
	{L"tab",			L"label"		},
	{L"tab",			L"caption"		},
	{L"url",			L"text"			},
	{L"url",			L"url"			},
	{NULL,				NULL			}
};

void processNode(IXMLDOMNode* root)
{
	BSTR nodeName = NULL;
	root->get_nodeName(&nodeName);
	for(int i=0; tags[i].tag; i++)
	{
		if(!lstrcmpi(tags[i].tag, nodeName))
		{
			LPTSTR val = xmlGetAttributeSTR(root, tags[i].attr);
			if(val)
			{
				LOC_STR locstr = {0};
				
				locstr.id				= xmlGetAttributeSTR(root, L"locid");
				locstr.defString		= val;
				locstr.node				= root;
				MAKE_STR(locstr.attr,	tags[i].attr);
				locstr.node->AddRef();
				gStrings.Add(&locstr);
			}
		}
	}
	SysFreeString(nodeName);
	IXMLDOMNode* child = NULL;
	root->get_firstChild(&child);
	while(child)
	{
		processNode(child);
		XML_NEXTNODE(child);
	}
}

void processFile(LPTSTR fileName, LPTSTR prefix)
{
	CComPtr<IXMLDOMDocument> doc;
	doc.CoCreateInstance(CLSID_DOMFreeThreadedDocument, NULL, CLSCTX_INPROC_SERVER);
	doc->put_async(VARIANT_FALSE);
	doc->put_validateOnParse(VARIANT_FALSE);
	doc->put_resolveExternals(VARIANT_FALSE);
	VARIANT_BOOL bSuccess = VARIANT_FALSE;
	if(doc->load(CComVariant(fileName), &bSuccess) == S_OK && bSuccess == VARIANT_TRUE)
	{
		IXMLDOMNode* root = xmlOpenNode(doc, TEXT("dialog"));
		if(root)
		{
			processNode(root);

			for(int i=0; i < gStrings.GetCount(); i++)
			{
				if(!gStrings[i].id)
				{
					gStrings[i].id	= xmlGetAttributeSTR(gStrings[i].node, L"locid");
					if(!gStrings[i].id)
					{
						for(int key=1; true; key++)
						{
							TCHAR id[255];
							wsprintf(id, L"%s-%X", prefix, key);
							BOOL found = FALSE;
							for(int j=0; j < gStrings.GetCount(); j++)
							{
								if(gStrings[j].id)
								{
									if(!lstrcmpi(id, gStrings[j].id))
									{
										found = TRUE;
										break;
									}
								}
							}
							if(!found)
							{
								MAKE_STR(gStrings[i].id, id);
								break;
							}
						}
						if(gStrings[i].node)
						{
							CComQIPtr<IXMLDOMElement> spXMLChildElement;
							spXMLChildElement = gStrings[i].node;
							spXMLChildElement->setAttribute(L"locid", CComVariant(gStrings[i].id));
							gStrings[i].node->Release();
							gStrings[i].node = NULL;
						}
					}
				}
			}

			root->Release();
		}
		doc->save(CComVariant(fileName));
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	CoInitialize(NULL);

	BOOL ret = FALSE;
	CComPtr<IXMLDOMDocument> doc;
	doc.CoCreateInstance(CLSID_DOMFreeThreadedDocument, NULL, CLSCTX_INPROC_SERVER);
	doc->put_async(VARIANT_FALSE);
	doc->put_validateOnParse(VARIANT_FALSE);
	doc->put_resolveExternals(VARIANT_FALSE);
	VARIANT_BOOL bSuccess = VARIANT_FALSE;
	if(doc->load(CComVariant(argv[1]), &bSuccess) == S_OK && bSuccess == VARIANT_TRUE)
	{
		IXMLDOMNode* root = xmlOpenNode(doc, TEXT("xuiproject"));
		if(root)
		{
			IXMLDOMNode* files = xmlOpenNode(root, TEXT("files"));
			if(files)
			{
				IXMLDOMNode* file = xmlOpenNode(files, TEXT("file"));
				while(file)
				{
					LPTSTR name = xmlGetAttributeSTR(file, TEXT("name"));
					if(name)
					{
						LPTSTR prefix = xmlGetAttributeSTR(file, TEXT("prefix"));
						TCHAR flName[MAX_PATH];
						lstrcpy(flName, argv[1]);
						PathRemoveFileSpec(flName);
						PathAddBackslash(flName);
						lstrcat(flName, name);
						processFile(flName, prefix);
						delete name;
						delete prefix;
					}
					XML_NEXTNODE(file);
				}
				files->Release();
			}
			IXMLDOMNode* nodeStrings = xmlOpenNode(root, TEXT("strings"));
			if(nodeStrings)
			{
				for(int i=0; i < gStrings.GetCount(); i++)
				{
					TCHAR nodeQuery[300];
					wsprintf(nodeQuery, L"string[@id=\"%s\" and @attr=\"%s\"]", gStrings[i].id, gStrings[i].attr);
					IXMLDOMNode* nodeStr = xmlOpenNode(nodeStrings, nodeQuery);
					if(!nodeStr)
					{
						IXMLDOMNode* newNode = NULL;
						HRESULT hr = doc->createNode(CComVariant(NODE_ELEMENT), CComBSTR(L"string"), NULL, &newNode);
						if(newNode)
						{
							IXMLDOMNode* insertedNode = NULL;
							nodeStrings->appendChild(newNode, &insertedNode);
							if(insertedNode)
							{
								CComQIPtr<IXMLDOMElement> spXMLChildElement;
								spXMLChildElement = insertedNode;
								spXMLChildElement->setAttribute(L"id",		CComVariant(gStrings[i].id));
								spXMLChildElement->setAttribute(L"attr",	CComVariant(gStrings[i].attr));
								spXMLChildElement->setAttribute(L"text",	CComVariant(gStrings[i].defString));
								insertedNode->Release();
							}
							newNode->Release();
						}
					}
				}
				nodeStrings->Release();
			}

			root->Release();
		}
		doc->save(CComVariant(argv[1]));
	}
	return 0;
}
