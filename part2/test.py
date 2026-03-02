import commit_retrieve as cr

ci = cr.retrieve_commit_info("../repos/ClickHouse", "32dcbfb6273fffacda1ec09c8cbf737f82ca0d04")

print(ci)