-- .nvim.lua
local map = vim.keymap.set

map("n", "<leader>rb", "<cmd>!./build-all.sh<CR>", { desc = "Build all" })

map("n", "<leader>rg", function()
	vim.cmd("botright split | terminal gdb -ex 'handle SIGPIPE nostop noprint pass' ./bin/project")
	vim.cmd("startinsert")
end, { desc = "Run under gdb" })

map("n", "<leader>rv", function()
	vim.cmd("botright split | terminal env LD_LIBRARY_PATH=./bin valgrind --leak-check=full ./bin/project")
	vim.cmd("startinsert")
end, { desc = "Run under valgrind" })
