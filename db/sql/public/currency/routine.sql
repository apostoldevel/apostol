--------------------------------------------------------------------------------
-- public.new_currency ---------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.new_currency (
  pId           uuid,
  pCode         text,
  pName         text
) RETURNS	    public.currency
AS $$
DECLARE
  r             public.currency%rowtype;
BEGIN
  INSERT INTO public.currency (id, code, name)
  VALUES (pId, pCode, pName)
  RETURNING * INTO r;

  RETURN r;
END;
$$ LANGUAGE plpgsql
   SECURITY DEFINER
   SET search_path = public, pg_temp;

--------------------------------------------------------------------------------
-- public.add_currency ---------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.add_currency (
  pCode         text,
  pName         text
) RETURNS	    uuid
AS $$
  SELECT id FROM public.new_currency(gen_random_uuid(), pCode, pName);
$$ LANGUAGE sql
   SECURITY DEFINER
   SET search_path = public, pg_temp;

--------------------------------------------------------------------------------
-- public.update_currency ------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.update_currency (
  pId           uuid,
  pCode         text,
  pName         text
) RETURNS	    bool
AS $$
BEGIN
  UPDATE public.currency
     SET code = coalesce(pCode, code),
         name = coalesce(pName, name)
   WHERE id = pId;
  RETURN FOUND;
END;
$$ LANGUAGE plpgsql
   SECURITY DEFINER
   SET search_path = public, pg_temp;

--------------------------------------------------------------------------------
-- public.get_currency_id ------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.get_currency_id (
  pCode         text
) RETURNS	    uuid
AS $$
  SELECT id FROM public.currency WHERE code = pCode;
$$ LANGUAGE sql
   SECURITY DEFINER
   SET search_path = public, pg_temp;

