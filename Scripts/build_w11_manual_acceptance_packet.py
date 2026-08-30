import argparse
import hashlib
import json
import sys
from pathlib import Path

from verify_w11_manual_acceptance_logs import (
    KEY_VALUE,
    load_and_validate_evidence,
    synthetic_self_test,
)
from w11_manual_acceptance_schema import (
    SUBJECTIVE_CHECKS,
    VISUAL_RESOLUTIONS,
    build_signoff_template,
)


OUTCOME_LABELS = {"1": "胜利", "2": "失败"}


def fail(message):
    print(f"W11_MANUAL_ACCEPTANCE_PACKET_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def player_facts(run):
    lines = [line for line in run.text.splitlines() if "W11_COMBAT_SUMMARY_PLAYER " in line]
    if len(lines) != 1:
        fail(f"{run.label} must contain exactly one standalone player summary, found {len(lines)}")
    return dict(KEY_VALUE.findall(lines[0]))


def evidence_hash(run):
    path = Path(run.label)
    data = path.read_bytes() if path.is_file() else run.text.encode("utf-8")
    return hashlib.sha256(data).hexdigest()


def escape_cell(value):
    return str(value).replace("|", "\\|").replace("\n", " ")


def build_report(
    runs,
    sects,
    outcomes,
    operator,
    signoff_name="W11人工单人验收签字.json",
):
    rows = []
    hashes = []
    for run in runs:
        player = player_facts(run)
        digest = evidence_hash(run)
        summary = run.summary
        rows.append(
            "| " + " | ".join(
                escape_cell(value)
                for value in (
                    run.role,
                    player.get("Hero", "?"),
                    player.get("Sect", "?"),
                    OUTCOME_LABELS.get(summary.get("Outcome"), summary.get("Outcome", "?")),
                    summary.get("Duration", "?"),
                    summary.get("Health", player.get("Health", "?")),
                    summary.get("DamageDealt", "?"),
                    summary.get("DamageTaken", "?"),
                    Path(run.label).name,
                )
            ) + " |"
        )
        hashes.append(f"- `{Path(run.label).name}`: `{digest}`")

    return "\n".join(
        [
            "# W11 人工单人验收记录",
            "",
            "> 客观日志门禁：**PASSED**  ",
            "> 主观验收状态：**PENDING — 必须由实际操作者填写并签字**  ",
            "> 本报告不会根据日志自动宣称画面、音频、舒适度或好玩程度合格。",
            "",
            "## 1. 会话信息",
            "",
            f"- 操作者：{operator or '待填写'}",
            "- 固定 Seed：424242",
            "- 日志 Schema：3",
            f"- 覆盖门派：{', '.join(sorted(sects))}",
            f"- 覆盖结果：{', '.join(OUTCOME_LABELS.get(value, value) for value in sorted(outcomes))}",
            f"- 证据运行数：{len(runs)}（必须包含 Observe、Evasion、Dodge、Recovery、Feedback）",
            "",
            "| 角色 | Hero | Sect | 结果 | 时长(s) | 终局气血 | 造成伤害 | 承受伤害 | 日志 |",
            "|---|---|---|---:|---:|---:|---:|---:|---|",
            *rows,
            "",
            "## 2. 已由日志证明的客观覆盖",
            "",
            "- [x] Observe：三类敌方攻击均进入 Active，且该局无攻击、无闪避。",
            "- [x] Evasion：只用移动，至少避开一次瘴气弹。",
            "- [x] Dodge：残剑魂冲斩产生至少一次闪避免疫。",
            "- [x] Recovery：后摇窗口造成至少一次权威生命伤害。",
            "- [x] Feedback：独立日志覆盖普攻、多目标术法、实际治疗、实际护障、会心、护障吸收和击败。",
            "- [x] 至少三个门派，并覆盖胜利与失败。",
            "- [x] 所有会话均为可视、有音频、手动 Standalone；无脚本、AutoStart 或测试覆写。",
            "- [x] DuplicateRewards、GhostDamage、NoWarningDamage、UnmatchedWarnings、Stuck 均为 0。",
            "",
            "## 3. 真人主观验收（必须填写）",
            "",
            "请在每项选择一个结果并填写具体观察；不能只勾选而没有备注。正式封存以同目录结构化签字文件为准。",
            "",
            "- [ ] 三类敌人的颜色、形状、动作与前摇声音能在首次遭遇时被区分。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] 瘴气弹路径和速度可读，纯移动躲避不依赖预知脚本。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] 残剑魂冲斩的预警窗口与空格闪避响应自然，未感到输入迟滞。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] 石甲重砸后摇清晰，玩家能凭画面判断输出窗口。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] 普攻、术法、受击、会心、护障与击败反馈层级清楚，不互相淹没。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] 镜头冲击和局部停顿在连续战斗中舒适，无眩晕或明显打断操作。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] 使用耳机能仅凭声音区分三类敌袭，整体响度与密度舒适。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] HUD 在 3840×2160 及三个兼容分辨率下清晰，无关键遮挡或裁切。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "- [ ] 约 90–150 秒首战节奏有攻防起伏，整体战斗值得继续游玩。",
            "  - 结果：通过 / 需调整",
            "  - 备注：",
            "",
            "## 4. 四档分辨率视觉证据（必须逐档采集）",
            "",
            "每档必须保存一张实际运行截图到会话目录的 `VisualEvidence` 子目录；封存器会读取 PNG IHDR，核对真实像素尺寸并计算 SHA-256。",
            "",
            "| 分辨率 | 结果 | 备注 | 默认文件 |",
            "|---|---|---|---|",
            *(
                f"| {resolution} | PENDING | 待填写 | `VisualEvidence/W11_{resolution}.png` |"
                for resolution in VISUAL_RESOLUTIONS
            ),
            "",
            "## 5. 测试环境与签字",
            "",
            "- 日期：",
            "- 操作设备与输入方式：",
            "- 显示器与测试分辨率：",
            "- 耳机 / 音频设备：",
            "- 平均帧率或明显卡顿：",
            "- 总体结论：通过 / 需调整",
            "- 操作者签字：",
            "",
            "## 6. 结构化签字与封存",
            "",
            f"- 待填写签字清单：`{signoff_name}`",
            "- 填写后运行：`python Scripts/finalize_w11_manual_acceptance_signoff.py --session session.json`",
            "- 只有封存器输出 `W11_MANUAL_ACCEPTANCE_SIGNOFF_FINALIZED`，主观状态才不再是 PENDING。",
            "",
            "## 7. 客观日志完整性（SHA-256）",
            "",
            *hashes,
            "",
            "若任一日志发生修改，必须重新运行客观门禁并重新生成本报告。",
            "",
        ]
    )


def main():
    parser = argparse.ArgumentParser(
        description="Validate five W11 manual roles and create an objective-evidence packet with human sign-off fields."
    )
    parser.add_argument("--observe", type=Path)
    parser.add_argument("--evasion", type=Path)
    parser.add_argument("--dodge", type=Path)
    parser.add_argument("--recovery", type=Path)
    parser.add_argument("--extra", action="append", type=Path, default=[])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--signoff-output", type=Path)
    parser.add_argument("--operator", default="")
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        runs, sects, outcomes = synthetic_self_test()
        report = build_report(runs, sects, outcomes, "SelfTest")
        template = build_signoff_template("SelfTest", "objective.md")
        for required in (
            "客观日志门禁：**PASSED**",
            "主观验收状态：**PENDING",
            "Feedback",
            "3840x2160",
            "finalize_w11_manual_acceptance_signoff.py",
            "SHA-256",
        ):
            if required not in report:
                fail(f"self-test report is missing {required}")
        if tuple(template["visualEvidence"]) != VISUAL_RESOLUTIONS:
            fail("self-test sign-off template does not contain the four required resolutions")
        if tuple(template["checks"]) != tuple(check_id for check_id, _ in SUBJECTIVE_CHECKS):
            fail("self-test sign-off template does not contain the required subjective checks")
        print("W11_MANUAL_ACCEPTANCE_PACKET_SELF_TEST_PASSED")
        return

    if not args.output:
        fail("--output is required")
    if args.output.exists() and not args.force:
        fail(f"output already exists: {args.output}; use --force to replace it")
    signoff_output = args.signoff_output or args.output.with_name("W11人工单人验收签字.json")
    if signoff_output.exists() and not args.force:
        fail(f"sign-off template already exists: {signoff_output}; use --force to replace it")
    runs, sects, outcomes = load_and_validate_evidence(
        args.observe, args.evasion, args.dodge, args.recovery, args.extra
    )
    report = build_report(runs, sects, outcomes, args.operator, signoff_output.name)
    signoff = build_signoff_template(args.operator, args.output.name)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(report, encoding="utf-8")
    signoff_output.parent.mkdir(parents=True, exist_ok=True)
    signoff_output.write_text(
        json.dumps(signoff, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(
        "W11_MANUAL_ACCEPTANCE_PACKET_CREATED: "
        f"Output={args.output.resolve()} Signoff={signoff_output.resolve()} "
        f"Runs={len(runs)} ObjectiveGate=PASSED Subjective=PENDING"
    )


if __name__ == "__main__":
    main()
