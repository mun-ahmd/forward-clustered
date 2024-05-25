inputFile = 'enumList.txt'
outputFile = 'outputEnums.txt'
prefix = 'VK_COMPARE_OP_'

enums = ''
with open(inputFile, 'r') as file:
    enums = file.read()

def snakeToCamel(snake : str):
    camel = ""
    isNextCap = False
    for c in snake:
        if c == '_':
            isNextCap = True
        else:
            if isNextCap:
                camel += c.upper()
                isNextCap = False
            else:
                camel += c.lower()
    return camel

enumFromName = {
    snakeToCamel(
        x.strip().split('=')[0].strip().removeprefix(prefix)
    ) : 
    x.strip().split('=')[0]
    for x in enums.split(',')
}

output = []
for name, enumName in enumFromName.items():
    if name.strip() == "":
        continue
    output.append(f'\t{{"{name}", {enumName}}},')
output[-1] = output[-1][0:-1]
output.insert(0, "{")
output.append("}")
    
with open(outputFile, 'w') as file:
    file.writelines(out + '\n' for out in output)

print(output)
