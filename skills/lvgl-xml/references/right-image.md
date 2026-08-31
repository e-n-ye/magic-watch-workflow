# Right image drafts

This reference documents the repository wrapper around Right's asynchronous
OpenAI Images-compatible endpoint. It is for exploratory UI concepts only.

## Safety and approval

The helper never sends a request unless `--confirm-paid` is present. Run the
same command with `--dry-run` first, show its printed summary to the user, and
wait for approval of model, count, size, references, and prompt. Put the key in
`.env.local` (ignored by Git) or export `RIGHT_API_KEY`; never paste it into
chat or commit it. Copy `.env.example` to `.env.local` and fill the value
locally.

## Request contract

- Submit: `POST https://www.rightapi.ai/draw/v1/images/generations`
- Header: `Authorization: Bearer <key>` and JSON content type
- Required body fields: `model`, `prompt`, and `"async": true`
- Draft defaults: `gpt-image-2`, `n=3`, `size=9:16`, `imageSize=1K`
- Optional references: `image` is an array of `data:<mime>;base64,...` URLs
- Poll: `GET https://www.rightapi.ai/v1/tasks/{task_id}` (no `/draw` prefix)
- Terminal success: `completed` or a response containing `data`/`candidates`
- Terminal failure: `failed`, `error`, or `cancelled`; preserve the message in
  local logs but do not place credentials in the manifest

The helper accepts URL or base64 image results, writes `draft-01.png` and
siblings, and records task id, request summary, output hashes, and byte sizes
in a redacted JSON manifest. It does not require third-party Python packages.

## Commands

Preview a paid request without network access:

```sh
python skills/lvgl-xml/scripts/right_image_job.py \\
  --prompt "modern tool watch face, large time, date, battery, sensor summary" \\
  --model gpt-image-2 --n 3 --size 9:16 --image-size 1K \\
  --output-dir build/design-evidence/watchface-v1 --dry-run
```

After explicit approval, submit and collect results:

```sh
python skills/lvgl-xml/scripts/right_image_job.py \\
  --prompt "<the approved brief>" --model gpt-image-2 --n 3 \\
  --size 9:16 --image-size 1K \\
  --output-dir build/design-evidence/watchface-v1 --confirm-paid
```

Use `--model gpt-image-2-vip` only when the user explicitly approves a VIP
refinement. Increase `--image-size` only when the visual decision requires it;
large outputs are not suitable as F411 full-screen backgrounds by default.

## Evidence handoff

Keep generated drafts, thumbnails, task responses, and confirmation notes under
`build/design-evidence/` (ignored by the repository's `build` rule). Once a
variant is approved, record its filename and SHA-256 in the handoff note, then
translate the layout into XML. The concept image remains a reference; XML is
the source of truth for production UI.