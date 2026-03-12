from langchain_core.prompts import ChatPromptTemplate
import utils

def optimize(mission: utils.Mission) -> tuple[str, str]:
    """
    Optimizes the provided C/C++ code using an LLM with Chain of Thought (CoT).
    
    The goal is strictly to improve runtime efficiency (speed) or reduce resource consumption.
    The function returns a tuple: (optimized_code, optimization_reason).
    It uses LangChain with the configured XMCP model.
    """

    # Define the prompt that encourages CoT and code optimization
    system_prompt = (
        f"{utils.system_prompt_role_prelog}"
        "You MUST use Chain of Thought (CoT) reasoning:\n"
        "1. Analyze the original code and the specific function to identify bottlenecks.\n"
        "2. Propose optimizations for the specific function. Prioritize simple, high-impact changes over complex algorithmic rewrites unless necessary.\n"
        "3. Select EXACTLY ONE optimization idea to apply. Even if multiple valid ideas exist, pick only the most impactful one.\n"
        "4. Apply the chosen optimization to the function. Changes should be as simple as possible and must NOT modify the overall logic structure drastically.\n\n"
        "Output Format:\n"
        f"{utils.system_prompt_final_output_format}"
    )
    
    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{user_prompt}")
    ])
    
    # Create the chain
    chain = prompt | utils.llm
    
    # Execute the chain
    try:
        response = chain.invoke({"user_prompt": mission.show_goal()})
        content = response.content
        return utils.parse_final_output(content)
    except Exception as e:
        print(f"Error during optimization: {e}")
        raise e
