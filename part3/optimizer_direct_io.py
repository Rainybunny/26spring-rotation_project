import os
import sys
import re
from pathlib import Path

# Add project root to sys.path to allow importing config
sys.path.append(str(Path(__file__).parent.parent.absolute()))
import config

from langchain_openai import ChatOpenAI
from langchain_core.prompts import ChatPromptTemplate

def optimize(user_prompt: str) -> tuple[str, str]:
    """
    Optimizes the provided C/C++ code using an LLM with Chain of Thought (CoT).
    
    The goal is strictly to improve runtime efficiency (speed) or reduce resource consumption.
    The function returns a tuple: (optimized_code, optimization_reason).
    It uses LangChain with the configured XMCP model.
    """
    
    # Initialize the LLM with the configuration from config.py
    llm = ChatOpenAI(
        openai_api_key=config.xmcp_api_key,
        openai_api_base=config.xmcp_base_url,
        model_name=config.xmcp_model,
        temperature=0.0  # Lower temperature for deterministic code generation
    )
    
    # Define the prompt that encourages CoT and code optimization
    system_prompt = (
        "You are an expert C/C++ optimization engineer. "
        "Your goal is to optimize a specific function within the provided code for maximum runtime efficiency and minimal resource usage.\n"
        "Do NOT strictly focus on readability or style; focus on performance.\n\n"
        "You MUST use Chain of Thought (CoT) reasoning:\n"
        "1. Analyze the original code and the specific function to identify bottlenecks. Pay special attention to:\n"
        "   - Unnecessary object copying (e.g., in loops, function arguments, or lambda captures).\n"
        "   - Redundant memory allocations.\n"
        "   - Inefficient loop logic.\n"
        "2. Propose optimizations for the specific function. Prioritize simple, high-impact changes (like adding 'const &' or '&') over complex algorithmic rewrites unless necessary.\n"
        "3. Select EXACTLY ONE optimization idea to apply. Even if multiple valid ideas exist, pick only the most impactful one.\n"
        "4. Apply the chosen optimization to the function. Changes should be as simple as possible and must NOT modify the overall logic structure drastically.\n\n"
        "Output Format:\n"
        "First, output your thought process/analysis.\n"
        "Second, output a clear description of the final optimization idea.\n"
        "Then, output the final optimized COMPETE FUNCTION IMPLEMENTATION inside a single markdown code block (```cpp ... ``` or ```c ... ```).\n"
        "The code block must contain ONLY the optimized function. It must be a direct replacement for the original function."
    )
    
    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{user_prompt}")
    ])
    
    # Create the chain
    chain = prompt | llm
    
    # Execute the chain
    try:
        response = chain.invoke({"user_prompt": user_prompt})
        content = response.content

        # Extract the logical code block from the response
        # Look for the last code block as it likely contains the final result
        # Regex for ```cpp or ```c or ```C++
        pattern = r"```(?:c|cpp|C\+\+|cc)?\s*\n(.*?)```"
        # Use finditer to get match objects with positions
        matches = list(re.finditer(pattern, content, re.DOTALL | re.IGNORECASE))

        if matches:
            last_match = matches[-1]
            code = last_match.group(1).strip()
            # Everything before the start of the code block is the reasoning
            reason = content[:last_match.start()].strip()
            return code, reason

        # Fallback: if no specific block found, check for generic block
        pattern_generic = r"```\s*\n(.*?)```"
        matches_generic = list(re.finditer(pattern_generic, content, re.DOTALL))
        
        if matches_generic:
            last_match = matches_generic[-1]
            code = last_match.group(1).strip()
            reason = content[:last_match.start()].strip()
            return code, reason

        # If no code blocks found at all, return the raw content 
        # (though this is less ideal, it's better than empty)
        return content, "No code block found in response."

    except Exception as e:
        # In case of error (e.g. API issues), log and re-raise
        print(f"Error during optimization: {e}", file=sys.stderr)
        raise e
