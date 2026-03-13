TRICKS

after having designed the pagte and exported the Mongoose folder back into this tproject, must take out STATIC in mongoose_glue.c
replace "static " for "" without ""  
For monggoose log levels set to >0 in cmakelist.txt (src code not main)

target_compile_definitions(${COMPONENT_LIB} PRIVATE MG_ENABLE_LOG=0)

