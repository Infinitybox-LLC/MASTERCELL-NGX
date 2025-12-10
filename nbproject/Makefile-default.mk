#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=main.c lcd.c buttons.c inputs.c j1939.c eeprom_cases.c eeprom_init.c eeprom_config.c can_config.c network_inventory.c inlink.c eeprom_init_front_engine.c eeprom_init_rear_engine.c eeprom_init_customer.c climate.c outputs.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/main.o ${OBJECTDIR}/lcd.o ${OBJECTDIR}/buttons.o ${OBJECTDIR}/inputs.o ${OBJECTDIR}/j1939.o ${OBJECTDIR}/eeprom_cases.o ${OBJECTDIR}/eeprom_init.o ${OBJECTDIR}/eeprom_config.o ${OBJECTDIR}/can_config.o ${OBJECTDIR}/network_inventory.o ${OBJECTDIR}/inlink.o ${OBJECTDIR}/eeprom_init_front_engine.o ${OBJECTDIR}/eeprom_init_rear_engine.o ${OBJECTDIR}/eeprom_init_customer.o ${OBJECTDIR}/climate.o ${OBJECTDIR}/outputs.o
POSSIBLE_DEPFILES=${OBJECTDIR}/main.o.d ${OBJECTDIR}/lcd.o.d ${OBJECTDIR}/buttons.o.d ${OBJECTDIR}/inputs.o.d ${OBJECTDIR}/j1939.o.d ${OBJECTDIR}/eeprom_cases.o.d ${OBJECTDIR}/eeprom_init.o.d ${OBJECTDIR}/eeprom_config.o.d ${OBJECTDIR}/can_config.o.d ${OBJECTDIR}/network_inventory.o.d ${OBJECTDIR}/inlink.o.d ${OBJECTDIR}/eeprom_init_front_engine.o.d ${OBJECTDIR}/eeprom_init_rear_engine.o.d ${OBJECTDIR}/eeprom_init_customer.o.d ${OBJECTDIR}/climate.o.d ${OBJECTDIR}/outputs.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/main.o ${OBJECTDIR}/lcd.o ${OBJECTDIR}/buttons.o ${OBJECTDIR}/inputs.o ${OBJECTDIR}/j1939.o ${OBJECTDIR}/eeprom_cases.o ${OBJECTDIR}/eeprom_init.o ${OBJECTDIR}/eeprom_config.o ${OBJECTDIR}/can_config.o ${OBJECTDIR}/network_inventory.o ${OBJECTDIR}/inlink.o ${OBJECTDIR}/eeprom_init_front_engine.o ${OBJECTDIR}/eeprom_init_rear_engine.o ${OBJECTDIR}/eeprom_init_customer.o ${OBJECTDIR}/climate.o ${OBJECTDIR}/outputs.o

# Source Files
SOURCEFILES=main.c lcd.c buttons.c inputs.c j1939.c eeprom_cases.c eeprom_init.c eeprom_config.c can_config.c network_inventory.c inlink.c eeprom_init_front_engine.c eeprom_init_rear_engine.c eeprom_init_customer.c climate.c outputs.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=30F6012A
MP_LINKER_FILE_OPTION=,--script=p30F6012A.gld
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/main.o: main.c  .generated_files/flags/default/7d1eaf04d89f681bfb12171292d6169ed17e540e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lcd.o: lcd.c  .generated_files/flags/default/77e26150ca4b99be387c1ac99cd12feabf80a7a1 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/lcd.o.d 
	@${RM} ${OBJECTDIR}/lcd.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lcd.c  -o ${OBJECTDIR}/lcd.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lcd.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/buttons.o: buttons.c  .generated_files/flags/default/ee7be11469ae607fd3a3ab0855bdb770c3494d0d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/buttons.o.d 
	@${RM} ${OBJECTDIR}/buttons.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  buttons.c  -o ${OBJECTDIR}/buttons.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/buttons.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/inputs.o: inputs.c  .generated_files/flags/default/66a5f1a0dc05dbe827927afd3928f263e85d99c7 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/inputs.o.d 
	@${RM} ${OBJECTDIR}/inputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  inputs.c  -o ${OBJECTDIR}/inputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/inputs.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/j1939.o: j1939.c  .generated_files/flags/default/e4ee040a1e980d28ba6e687fe801d05a95a015d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/j1939.o.d 
	@${RM} ${OBJECTDIR}/j1939.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  j1939.c  -o ${OBJECTDIR}/j1939.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/j1939.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_cases.o: eeprom_cases.c  .generated_files/flags/default/32be840e1b0afbc1c328ce2249a6a195c3ce4e9 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_cases.o.d 
	@${RM} ${OBJECTDIR}/eeprom_cases.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_cases.c  -o ${OBJECTDIR}/eeprom_cases.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_cases.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init.o: eeprom_init.c  .generated_files/flags/default/ff8be1da7c4683e5771e438085c59281b9f93a47 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init.c  -o ${OBJECTDIR}/eeprom_init.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_config.o: eeprom_config.c  .generated_files/flags/default/a4afe4411be9fb909a68efb894cde7bacd8bbc96 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_config.o.d 
	@${RM} ${OBJECTDIR}/eeprom_config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_config.c  -o ${OBJECTDIR}/eeprom_config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_config.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/can_config.o: can_config.c  .generated_files/flags/default/aaed17fd0b3e1a5f3bde35b678b4053fb715021e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/can_config.o.d 
	@${RM} ${OBJECTDIR}/can_config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  can_config.c  -o ${OBJECTDIR}/can_config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/can_config.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/network_inventory.o: network_inventory.c  .generated_files/flags/default/f5b3b25c742e016a76eec8e3c06cbf2710c26a56 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/network_inventory.o.d 
	@${RM} ${OBJECTDIR}/network_inventory.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  network_inventory.c  -o ${OBJECTDIR}/network_inventory.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/network_inventory.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/inlink.o: inlink.c  .generated_files/flags/default/25710c022375f21c31df6014bd2da002df62df8d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/inlink.o.d 
	@${RM} ${OBJECTDIR}/inlink.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  inlink.c  -o ${OBJECTDIR}/inlink.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/inlink.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init_front_engine.o: eeprom_init_front_engine.c  .generated_files/flags/default/5a61d1c79dd44767789943381c0d662640d3c64d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init_front_engine.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init_front_engine.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init_front_engine.c  -o ${OBJECTDIR}/eeprom_init_front_engine.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init_front_engine.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init_rear_engine.o: eeprom_init_rear_engine.c  .generated_files/flags/default/bb468a91490dada736ba848c284e700a2f48ed58 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init_rear_engine.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init_rear_engine.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init_rear_engine.c  -o ${OBJECTDIR}/eeprom_init_rear_engine.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init_rear_engine.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init_customer.o: eeprom_init_customer.c  .generated_files/flags/default/a44b99980f41a12852b1c80576650beb53de5b17 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init_customer.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init_customer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init_customer.c  -o ${OBJECTDIR}/eeprom_init_customer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init_customer.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/climate.o: climate.c  .generated_files/flags/default/bc7728c1e02e66d86c8c904eddbb2d277a285754 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/climate.o.d 
	@${RM} ${OBJECTDIR}/climate.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  climate.c  -o ${OBJECTDIR}/climate.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/climate.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/outputs.o: outputs.c  .generated_files/flags/default/9059ce0b774a3639f8a8f31d936e4fbc1ff12492 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/outputs.o.d 
	@${RM} ${OBJECTDIR}/outputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  outputs.c  -o ${OBJECTDIR}/outputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/outputs.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
else
${OBJECTDIR}/main.o: main.c  .generated_files/flags/default/a3deb946f30e57cf93bd36ab405dd1431d774f5 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lcd.o: lcd.c  .generated_files/flags/default/506514f53cb3a6d0e4224817500e58569cb8ffb3 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/lcd.o.d 
	@${RM} ${OBJECTDIR}/lcd.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lcd.c  -o ${OBJECTDIR}/lcd.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lcd.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/buttons.o: buttons.c  .generated_files/flags/default/36c64f11ff020a29bfb2e7a8afa0d55d74d5f0eb .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/buttons.o.d 
	@${RM} ${OBJECTDIR}/buttons.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  buttons.c  -o ${OBJECTDIR}/buttons.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/buttons.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/inputs.o: inputs.c  .generated_files/flags/default/569032a033d2c1a09fb9877ad412316842963762 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/inputs.o.d 
	@${RM} ${OBJECTDIR}/inputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  inputs.c  -o ${OBJECTDIR}/inputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/inputs.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/j1939.o: j1939.c  .generated_files/flags/default/c60b71f5f62b03fae610e5752084963856bea8cc .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/j1939.o.d 
	@${RM} ${OBJECTDIR}/j1939.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  j1939.c  -o ${OBJECTDIR}/j1939.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/j1939.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_cases.o: eeprom_cases.c  .generated_files/flags/default/3ea831c521bed79a737706bc34d4d2e68e3f5c0c .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_cases.o.d 
	@${RM} ${OBJECTDIR}/eeprom_cases.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_cases.c  -o ${OBJECTDIR}/eeprom_cases.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_cases.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init.o: eeprom_init.c  .generated_files/flags/default/d7546d2150beb90ad5de82f4347d5942a34bbea1 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init.c  -o ${OBJECTDIR}/eeprom_init.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_config.o: eeprom_config.c  .generated_files/flags/default/e7369dde399bc036d332990d920de33a64f92c .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_config.o.d 
	@${RM} ${OBJECTDIR}/eeprom_config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_config.c  -o ${OBJECTDIR}/eeprom_config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_config.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/can_config.o: can_config.c  .generated_files/flags/default/16540cd779efe4717bb76cf5ec937cb809c27894 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/can_config.o.d 
	@${RM} ${OBJECTDIR}/can_config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  can_config.c  -o ${OBJECTDIR}/can_config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/can_config.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/network_inventory.o: network_inventory.c  .generated_files/flags/default/c94de082cf4e386e6f4c078244ae212e734e7e82 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/network_inventory.o.d 
	@${RM} ${OBJECTDIR}/network_inventory.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  network_inventory.c  -o ${OBJECTDIR}/network_inventory.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/network_inventory.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/inlink.o: inlink.c  .generated_files/flags/default/d086f78d7499783070172f063aeac2a346c79422 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/inlink.o.d 
	@${RM} ${OBJECTDIR}/inlink.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  inlink.c  -o ${OBJECTDIR}/inlink.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/inlink.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init_front_engine.o: eeprom_init_front_engine.c  .generated_files/flags/default/3155c976aeb0ceeb66b1752915e1af3f5913f6a8 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init_front_engine.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init_front_engine.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init_front_engine.c  -o ${OBJECTDIR}/eeprom_init_front_engine.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init_front_engine.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init_rear_engine.o: eeprom_init_rear_engine.c  .generated_files/flags/default/cd7ac4470672cc2b373a3386b95e3b0201b8979a .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init_rear_engine.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init_rear_engine.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init_rear_engine.c  -o ${OBJECTDIR}/eeprom_init_rear_engine.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init_rear_engine.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_init_customer.o: eeprom_init_customer.c  .generated_files/flags/default/d2f8fcdaf90efc7f314f3df66260144de28d0765 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_init_customer.o.d 
	@${RM} ${OBJECTDIR}/eeprom_init_customer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_init_customer.c  -o ${OBJECTDIR}/eeprom_init_customer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_init_customer.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/climate.o: climate.c  .generated_files/flags/default/2e53dbb5898270656ea7243979d1eed5ce748a2b .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/climate.o.d 
	@${RM} ${OBJECTDIR}/climate.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  climate.c  -o ${OBJECTDIR}/climate.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/climate.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/outputs.o: outputs.c  .generated_files/flags/default/c2f12033498df4cf1ed5dbc650deaab0682d2994 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/outputs.o.d 
	@${RM} ${OBJECTDIR}/outputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  outputs.c  -o ${OBJECTDIR}/outputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/outputs.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -msmall-data -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o ${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG=__DEBUG   -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x84F   -Wl,,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,${DISTDIR}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	
else
${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o ${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,${DISTDIR}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	${MP_CC_DIR}/xc16-bin2hex ${DISTDIR}/MCNGX_Test.X_4.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf   -mdfp="${DFP_DIR}/xc16" 
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
