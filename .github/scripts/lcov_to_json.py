import sys
import json

"""
[
    {
        filename:"aaa.cpp"
        functions:[
            {
                function_name:"aaa:bbb()",
                hit_count:67
            }
        ]
    }
]
"""

def main():
    if len(sys.argv) != 3:
        print("Usage: python generate_csv.py <path/to/lcov/file> <path/to/json/file>")
        sys.exit(1)

    lcov_file_path = sys.argv[1]
    json_file_path = sys.argv[2]

    current_file = None
    file_dict = {}
    func_dict = {}
    func_line_dict = {}
    with open(lcov_file_path, "r") as f:
        for line in f:
            if line.startswith("end_of_record"):
                file_dict[current_file] = func_dict
                current_file = None
                func_dict = None
                func_line_dict = None
                continue
            if line.startswith("SF:"):
                current_file = line.strip().split(":", 1)[1]
                func_dict = {}
                func_line_dict = {}
            if not current_file:
                continue
    
            # TN: test name
            # SF: source file path
            # FN: line number,function name
            # FNF:  number functions found
            # FNH: number hit
            # BRDA: branch data: line, block, (expressions,count)+
            # BRF: branches found
            # DA: line number, hit count
            # LF: lines found
            # LH:  lines hit.

            if line.startswith("FN:"):
                # FN:line_number,function_name
                parts = line.strip().split(":", 1)[1].split(",")
                line_number, func_name = int(parts[0]), str(parts[1])
                func_line_dict[line_number] = func_name
                func_dict[func_name] = {"hit_count":0,"line_number":line_number}
            elif line.startswith("DA:"):
                # DA:line_number,hit_count
                parts = line.strip().split(":", 1)[1].split(",")
                line_number, hit_count = int(parts[0]), int(parts[1])

                if line_number in func_line_dict:
                    func_name = func_line_dict[line_number]
                    func_dict[func_name] = {"hit_count":hit_count, "line_number":line_number}
            # end if
        # end for line
    # end with open

    output_a = []
    #print("filename,function,hit_count")
    for filename in file_dict.keys():
        output_func_a = []
        func_dict = file_dict[filename]
        for func_name in func_dict.keys():
            f_d = func_dict[func_name]
            f_d["function_name"] = func_name
            output_func_a.append(f_d)
        file_d = {}
        file_d["filename"] = filename
        file_d["function_coverage"] = output_func_a
        output_a.append(file_d)

    
    with open(json_file_path, "w") as f:
        json.dump(output_a, f, indent=2)
        print(f"Wrote out to {json_file_path}")

# end main

if __name__ == "__main__":
    main()