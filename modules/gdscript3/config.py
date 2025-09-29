def can_build(env, platform):
    #env.module_add_dependencies("gdscript3", ["jsonrpc", "websocket"], True)
    env.module_add_dependencies("gdscript3", [], True)
    return True


def configure(env):
    pass


#def get_doc_classes():
#    return [
#        "@GDScript3",
#        "GDScript3SyntaxHighlighter",
#    ]
#
#
#def get_doc_path():
#    return "doc_classes"
