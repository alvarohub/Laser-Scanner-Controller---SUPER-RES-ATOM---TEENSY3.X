Import("env")

# access to global construction environment
print("environment: ")
print(env)

# Dump construction environment (for debug purpose)
print(env.Dump())

# append extra flags to global build environment
# which later will be used to build:
# - project source code
# - frameworks
# - dependent libraries
# env.Append(CPPDEFINES=[
#   "MACRO_1_NAME",
#   ("MACRO_2_NAME", "MACRO_2_VALUE")
# ])
